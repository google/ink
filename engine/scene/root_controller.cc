// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/engine/scene/root_controller.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <string>

#include "third_party/absl/memory/memory.h"
#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/brushes/tool_type.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/debug_view/debug_view.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/gl.h"
#include "ink/engine/processing/element_converters/bezier_path_converter.h"
#include "ink/engine/processing/element_converters/bundle_proto_converter.h"
#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/processing/element_converters/mesh_converter.h"
#include "ink/engine/processing/element_converters/scene_element_adder.h"
#include "ink/engine/processing/element_converters/stroke_outline_converter.h"
#include "ink/engine/processing/element_converters/text_mesh_converter.h"
#include "ink/engine/processing/runner/sequence_point_task.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/realtime/crop_tool.h"
#include "ink/engine/realtime/edit_tool.h"
#include "ink/engine/realtime/filter_chooser_tool.h"
#include "ink/engine/realtime/line_tool.h"
#include "ink/engine/realtime/magic_eraser.h"
#include "ink/engine/realtime/pusher_tool.h"
#include "ink/engine/realtime/query_tool.h"
#include "ink/engine/realtime/smart_highlighter_tool.h"
#include "ink/engine/realtime/stroke_editing_eraser.h"
#include "ink/engine/realtime/text_highlighter_tool.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/compositing/live_renderer.h"
#include "ink/engine/rendering/export/image_exporter.h"
#include "ink/engine/rendering/gl_managers/text_texture_provider.h"
#include "ink/engine/scene/data/common/serialized_element.h"
#include "ink/engine/scene/graph/element_notifier.h"
#include "ink/engine/scene/root_renderer.h"
#include "ink/engine/scene/types/element_bundle.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/security.h"
#include "ink/engine/util/time/logging_perf_timer.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/timer.h"
#include "ink/engine/util/time/wall_clock.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/export_portable_proto.pb.h"
#include "ink/proto/sengine_portable_proto.pb.h"

using ink::status::InvalidArgument;
using std::move;
using std::shared_ptr;
using std::unique_ptr;

namespace ink {
namespace {

class ReplaceTask : public Task {
 public:
  ReplaceTask(std::weak_ptr<SceneGraph> weak_scene_graph,
              const proto::ElementBundleReplace& replace_proto,
              const SourceDetails& source_details)
      : weak_scene_graph_(weak_scene_graph),
        replace_proto_(replace_proto),
        source_details_(source_details) {
    if (auto scene_graph = weak_scene_graph_.lock()) {
      callback_flags_ =
          scene_graph->GetElementNotifier()->GetCallbackFlags(source_details);

      const auto& proto_adds = replace_proto_.elements_to_add();
      scene_ids_.resize(proto_adds.element_bundle_add_size());
      for (int i = 0; i < proto_adds.element_bundle_add_size(); ++i) {
        const auto& add = proto_adds.element_bundle_add(i);
        ElementId id;
        if (scene_graph->GetNextPolyId(add.element_bundle().uuid(), &id)) {
          scene_ids_[i].id = id;
          scene_ids_[i].id_to_add_below =
              add.below_uuid() == kInvalidUUID
                  ? kInvalidElementId
                  : scene_graph->ElementIdFromUUID(add.below_uuid());
          scene_ids_[i].group_id =
              add.element_bundle().group_uuid() == kInvalidUUID
                  ? kInvalidElementId
                  : scene_graph->GroupIdFromUUID(
                        add.element_bundle().group_uuid());
        } else {
          SLOG(SLOG_WARNING, "Cannot add element bundle $0",
               add.element_bundle().uuid());
        }
      }

      const auto& proto_removes = replace_proto_.elements_to_remove();
      elements_to_remove_.reserve(proto_removes.uuid_size());
      for (const auto& uuid : proto_removes.uuid()) {
        auto id = scene_graph->ElementIdFromUUID(uuid);
        if (id != kInvalidElementId) elements_to_remove_.emplace_back(id);
      }
    }
  }

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}
  void Execute() override {
    elements_to_add_.reserve(scene_ids_.size());
    for (int i = 0; i < scene_ids_.size(); ++i) {
      const auto& bundle = replace_proto_.elements_to_add()
                               .element_bundle_add(i)
                               .element_bundle();
      BundleProtoConverter converter(bundle);
      auto processed_element =
          converter.CreateProcessedElement(scene_ids_[i].id);
      if (processed_element == nullptr) continue;
      processed_element->group = scene_ids_[i].group_id;

      auto serialized_element = absl::make_unique<SerializedElement>(
          bundle.uuid(), bundle.group_uuid(), source_details_, callback_flags_);
      serialized_element->Serialize(*processed_element);
      elements_to_add_.emplace_back(std::move(processed_element),
                                    std::move(serialized_element),
                                    scene_ids_[i].id_to_add_below);
    }
  }
  void OnPostExecute() override {
    if (auto scene_graph = weak_scene_graph_.lock())
      scene_graph->ReplaceElements(std::move(elements_to_add_),
                                   elements_to_remove_, source_details_);
  }

 private:
  struct SceneIds {
    ElementId id;
    ElementId id_to_add_below;
    GroupId group_id;
  };
  std::weak_ptr<SceneGraph> weak_scene_graph_;
  proto::ElementBundleReplace replace_proto_;
  SourceDetails source_details_;
  CallbackFlags callback_flags_;
  std::vector<SceneIds> scene_ids_;
  std::vector<SceneGraph::ElementAdd> elements_to_add_;
  std::vector<ElementId> elements_to_remove_;
};

}  // namespace

RootController::RootController(
    shared_ptr<IPlatform> platform,
    std::unique_ptr<service::DefinitionList> service_definitions)
    : size_(0, 0) {
#if INK_DEBUG
  SLOG(SLOG_INFO, "INK_DEBUG=1");
#else
  SLOG(SLOG_INFO, "INK_DEBUG=0");
#endif

  // create all registry
  service_definitions->DefineExistingService<IPlatform>(platform);
  registry_ = absl::make_unique<service::UncheckedRegistry>(
      std::move(service_definitions));

  // Grab refs to the registry we explicitly use. By doing this we give errors
  // at construction time vs use time.
  input_ = registry_->GetShared<input::InputDispatch>();
  tools_ = registry_->GetShared<ToolController>();
  frame_state_ = registry_->GetShared<FrameState>();
  graph_renderer_ = registry_->GetShared<LiveRenderer>();
  camera_controller_ = registry_->GetShared<CameraController>();
  platform_ = registry_->GetShared<IPlatform>();
  camera_ = registry_->GetShared<Camera>();
  scene_graph_ = registry_->GetShared<SceneGraph>();
  page_bounds_ = registry_->GetShared<PageBounds>();
  gl_resources_ = registry_->GetShared<GLResourceManager>();
  task_runner_ = registry_->GetShared<ITaskRunner>();
  root_renderer_ = registry_->GetShared<RootRenderer>();
  wall_clock_ = registry_->GetShared<WallClockInterface>();
  update_loop_ = registry_->GetShared<UpdateLoop>();
  grid_manager_ = registry_->GetShared<GridManager>();
  page_border_ = registry_->GetShared<PageBorder>();
  crop_mode_ = registry_->GetShared<CropMode>();
  flags_ = registry_->GetShared<settings::Flags>();
  flags_->AddListener(this);

  root_renderer_->AddDrawable(registry_->GetShared<DebugView>().get());

  draw_timer_ = absl::make_unique<LoggingPerfTimer>(wall_clock_, "draw time");
  blit_timer_ = absl::make_unique<LoggingPerfTimer>(wall_clock_, "blit time");

  SetupTools();

  unsafe_helper_ =
      std::unique_ptr<ink::UnsafeSceneHelper>(new ink::UnsafeSceneHelper(this));
  frame_limiter_ = absl::make_unique<FramerateLimiter>(*registry_);
}

void RootController::SetupTools() {
  tools_->AddTool(Tools::Line, std::unique_ptr<Tool>(new LineTool(*registry_)));
  tools_->AddTool(Tools::Query,
                  std::unique_ptr<Tool>(new QueryTool(*registry_)));
  tools_->AddTool(Tools::Edit,
                  std::unique_ptr<Tool>(new tools::EditTool(*registry_)));
  tools_->AddTool(Tools::MagicEraser,
                  std::unique_ptr<Tool>(new MagicEraser(*registry_)));
  tools_->AddTool(Tools::FilterChooser,
                  std::unique_ptr<Tool>(new FilterChooserTool(*registry_)));
  tools_->AddTool(Tools::Pusher,
                  std::unique_ptr<Tool>(new tools::PusherTool(*registry_)));
  tools_->AddTool(Tools::Crop,
                  std::unique_ptr<Tool>(new tools::CropTool(*registry_)));
  tools_->AddTool(Tools::TextHighlighterTool,
                  std::unique_ptr<Tool>(new TextHighlighterTool(*registry_)));
  tools_->AddTool(Tools::SmartHighlighterTool,
                  std::unique_ptr<Tool>(new SmartHighlighterTool(*registry_)));
  tools_->AddTool(
      Tools::StrokeEditingEraser,
      std::unique_ptr<Tool>(new tools::StrokeEditingEraser(*registry_)));

  tools_->SetToolType(Tools::Line);
}

RootController::~RootController() {
  SLOG(SLOG_OBJ_LIFETIME, "RootController dealloc");
}

void RootController::Reshape(const glm::ivec2 new_size, float ppi,
                             int rotation_deg) {
  camera_->SetPPI(ppi);
  camera_->SetScreenRotation(rotation_deg);
  if (size_ != new_size) {
    SLOG(SLOG_GL_STATE, "reshape - size changing $0->$1", size_, new_size);
    size_ = new_size;
    input_->ForceAllUp(*camera_);
    camera_->SetScreenDim(size_);
  }

  frame_state_->FrameEnd();
  graph_renderer_->Resize(size_);
  root_renderer_->Resize(size_, rotation_deg);

  camera_controller_->LookAt(camera_->WorldWindow());
}

Status RootController::SetToolParams(
    const ink::proto::ToolParams& unsafe_proto) {
  Tools::ToolType tool = static_cast<Tools::ToolType>(unsafe_proto.tool());

  INK_RETURN_UNLESS(
      BoundsCheckExEx(static_cast<int>(tool), Tools::ToolType::MinTool,
                      static_cast<int>(Tools::ToolType::MaxTool)));

  tools_->SetToolType(tool);
  glm::vec4 rgba = UintToVec4RGBA(static_cast<uint32_t>(unsafe_proto.rgba()));
  tools_->ChosenTool()->SetColor(rgba);

  bool pen_mode = flags_->GetFlag(settings::Flag::PenModeEnabled);

  float screen_width = camera_->ScreenDim().x;

  // If there are no page bounds, assume screen and world with are the same
  // for pt size computation.
  float world_width = screen_width;
  if (page_bounds_->HasBounds()) {
    world_width = page_bounds_->Bounds().Width();
  }

  if (tool == Tools::ToolType::Line) {
    if (!unsafe_proto.has_brush_type()) {
      ASSERT(false);
      return InvalidArgument("Line tool missing brush type!");
    }

    BrushParams params =
        BrushParams::GetBrushParams(unsafe_proto.brush_type(), pen_mode);

    if (unsafe_proto.has_line_size()) {
      INK_RETURN_UNLESS(BrushParams::BrushSize::PopulateSizeFromProto(
          unsafe_proto.line_size(), screen_width, camera_->GetPPI(),
          world_width, unsafe_proto.brush_type(), pen_mode, &params.size));
    }

    if (unsafe_proto.has_linear_path_animation()) {
      INK_RETURN_UNLESS(BrushParams::PopulateAnimationFromProto(
          unsafe_proto.linear_path_animation(), &params));
    }

    // Note that the Enabled tool will still not be LINE_TOOL here when we have
    // the ReadOnly flag on.
    LineTool* line_tool;
    if (tools_->GetTool(Tools::ToolType::Line, &line_tool)) {
      line_tool->SetBrushParams(params);
    }
  } else if (tool == Tools::ToolType::SmartHighlighterTool) {
    BrushParams params =
        BrushParams::GetBrushParams(proto::BrushType::HIGHLIGHTER, pen_mode);

    if (unsafe_proto.has_line_size()) {
      INK_RETURN_UNLESS(BrushParams::BrushSize::PopulateSizeFromProto(
          unsafe_proto.line_size(), screen_width, camera_->GetPPI(),
          world_width, unsafe_proto.brush_type(), pen_mode, &params.size));
    }

    SmartHighlighterTool* smart_highlighter;
    if (tools_->GetTool(Tools::ToolType::SmartHighlighterTool,
                        &smart_highlighter)) {
      smart_highlighter->SetBrushParams(params);
    }
  } else if (tool == Tools::ToolType::Pusher &&
             unsafe_proto.has_pusher_tool_params()) {
    tools::PusherTool* tool;
    if (tools_->GetTool(Tools::ToolType::Pusher, &tool)) {
      tool->SetPusherToolParams(unsafe_proto.pusher_tool_params());
    }
  } else if (tool == Tools::ToolType::StrokeEditingEraser) {
    tools::StrokeEditingEraser* tool;
    if (tools_->GetTool(Tools::ToolType::StrokeEditingEraser, &tool)) {
      BrushParams::BrushSize size;
      if (unsafe_proto.has_line_size()) {
        INK_RETURN_UNLESS(BrushParams::BrushSize::PopulateSizeFromProto(
            unsafe_proto.line_size(), screen_width, camera_->GetPPI(),
            world_width, unsafe_proto.brush_type(), pen_mode, &size));
      }
      tool->SetBrushSize(size);
    }
  }
  tool_params_ = unsafe_proto;
  return OkStatus();
}

void RootController::OnFlagChanged(settings::Flag which, bool new_value) {
  if (which == settings::Flag::PenModeEnabled) {
    // Every time pen mode changes, reinterpret the most recent tool params.
    if (tool_params_.brush_type() ==
        ink::proto::BrushType::BALLPOINT_IN_PEN_MODE_ELSE_MARKER) {
      SetToolParams(tool_params_).IgnoreError();
    }
  } else if (which == settings::Flag::DebugTiles) {
    auto tp = gl_resources_->texture_manager->GetTilePolicy();
    tp.debug_tiles = new_value;
    gl_resources_->texture_manager->SetTilePolicy(tp);
  } else if (which == settings::Flag::DebugLineToolMesh) {
    LineTool* line_tool;
    if (tools_->GetTool(Tools::ToolType::Line, &line_tool)) {
      line_tool->EnableDebugMesh(new_value);
    }
  }
}

void RootController::Draw(FrameTimeS draw_time) {
  SLOG(SLOG_DRAWING, "rootcontroller draw started");
  // NOTE(mrcasey): Camera and page bounds should not be modified during
  // drawing, as engine state has already been cached for this frame.
  frame_state_->FrameStart(draw_time);
  ASSERT(frame_state_->GetFrameNumber() == 1 ||
         frame_state_->GetFrameTime() > frame_state_->GetLastFrameTime());

  draw_timer_->Begin();

  // update
  update_loop_->Update(platform_->GetTargetFPS(), draw_time);

  // blit (draw to screen)
  blit_timer_->Begin();
  root_renderer_->Draw(draw_time);
  blit_timer_->End();

  frame_state_->FrameEnd();

  draw_timer_->End();

  SLOG(SLOG_DRAWING, "scene draw loop exiting");
}

void RootController::SetPageBorder(const std::string& border_uri, float scale) {
  page_border_->SetTexture(border_uri, scale);
}

void RootController::ClearPageBorder() { page_border_->ClearTexture(); }

void RootController::SetOutOfBoundsColor(glm::vec4 out_of_bounds_color) {
  gl_resources_->background_state->SetOutOfBoundsColor(out_of_bounds_color);
}

void RootController::SetGrid(const proto::GridInfo& grid_info) {
  grid_manager_->SetGrid(grid_info);
  // Request a frame, so that the grid changes show up.
  frame_state_->RequestFrame();
}

void RootController::ClearGrid() {
  grid_manager_->ClearGrid();
  // Request a frame, so that the grid changes show up.
  frame_state_->RequestFrame();
}

void RootController::AddElement(const ink::proto::ElementBundle& unsafe_bundle,
                                const SourceDetails& source_details) {
  AddElementBelow(unsafe_bundle, source_details, kInvalidUUID);
}

void RootController::AddElementBelow(
    const ink::proto::ElementBundle& unsafe_bundle,
    const SourceDetails& source_details, const UUID& below_element_with_uuid) {
  EXPECT(unsafe_bundle.has_uuid());
  auto uuid = unsafe_bundle.uuid();
  ElementId below_id = kInvalidElementId;
  if (below_element_with_uuid != kInvalidUUID) {
    // The element on top is allowed to be either a group or a poly.
    below_id = scene_graph_->ElementIdFromUUID(below_element_with_uuid);
  }
  // Note that we expect the group to exist at this point.
  UUID group_uuid = unsafe_bundle.group_uuid();
  GroupId group = kInvalidElementId;
  if (group_uuid != kInvalidUUID) {
    group = scene_graph_->GroupIdFromUUID(group_uuid);
    if (group == kInvalidElementId) {
      SLOG(SLOG_ERROR,
           "Group $0 not found for element $1. Using the root as the group.",
           group_uuid, uuid);
    }
  }

  SLOG(SLOG_DATA_FLOW,
       "RootController request to add uuid $0, belowUUID: $1, group: $2", uuid,
       below_element_with_uuid, group);

  if (unsafe_bundle.element().attributes().is_group()) {
    // Add the groups now instead of deferred to the end of frame via the
    // task manager. non GROUP elements require this group to exist due
    // to the group parent lookup.
    ElementId new_group_id;
    if (!scene_graph_->GetNextGroupId(uuid, &new_group_id)) {
      SLOG(SLOG_ERROR, "Could not get new group id for: $0", uuid);
      // Bail early. This will cause any attached child polys to attach to the
      // root.
      return;
    }
    glm::mat4 transform(1.0f);
    if (!ElementBundle::ReadObjectMatrix(unsafe_bundle, &transform)) {
      // If we fail to read the object to group matrix, use the identity.
      SLOG(SLOG_ERROR,
           "Could not read transform from group element; using the identity "
           "transform.");
      transform = glm::mat4(1.0f);
    }
    Rect empty_bounds;
    // Add the group without any bounds set and default unclippable. PageManager
    // will set the bounds and clippability when it generates the layout.
    scene_graph_->AddOrUpdateGroup(
        new_group_id, transform, empty_bounds, false,
        GroupTypeFromProto(unsafe_bundle.element().attributes().group_type()),
        source_details);
  } else if (unsafe_bundle.element().has_text()) {
    text::TextSpec text;
    if (!util::ReadFromProto(unsafe_bundle.element().text(), &text)) {
      SLOG(SLOG_ERROR, "Failed to read text proto.");
      return;
    }
    glm::mat4 transform(1.0f);
    if (!ElementBundle::ReadObjectMatrix(unsafe_bundle, &transform)) {
      SLOG(SLOG_ERROR, "Failed to read text transform");
      return;
    }

    // Text's object coordinates are always assumed to be kTextBoxSize x
    // kTextBoxSize, so the transform is from that rect into world coordinates.
    Rect object_rect = Rect(0, 0, kTextBoxSize, kTextBoxSize);

    AddTextRect(text, object_rect, group, uuid, transform, below_id);
  } else {
    // Note that the points bundle should already be in group coordinates so
    // a point -> group transform is not needed.
    unique_ptr<BundleProtoConverter> converter(
        new BundleProtoConverter(unsafe_bundle));
    unique_ptr<SceneElementAdder> adder_task(new SceneElementAdder(
        move(converter), scene_graph_, source_details, uuid, below_id, group));
    task_runner_->PushTask(move(adder_task));
  }
}

void RootController::AddStrokeOutline(
    const proto::StrokeOutline& unsafe_stroke_outline, const GroupId& group,
    const SourceDetails& source_details) {
  auto uuid = scene_graph_->GenerateUUID();

  SLOG(SLOG_DATA_FLOW,
       "RootController request to add StrokeOutline uuid $0 group $1", uuid,
       group);

  unique_ptr<StrokeOutlineConverter> converter(
      new StrokeOutlineConverter(unsafe_stroke_outline));
  unique_ptr<SceneElementAdder> adder_task(new SceneElementAdder(
      move(converter), scene_graph_, source_details, uuid,
      /* below_element_with_id= */ kInvalidElementId, group));
  task_runner_->PushTask(move(adder_task));
}

void RootController::RemoveElement(const UUID& uuid,
                                   const SourceDetails& source_details) {
  scene_graph_->RemoveElement(scene_graph_->ElementIdFromUUID(uuid),
                              source_details);
}

void RootController::ReplaceElements(const proto::ElementBundleReplace& replace,
                                     const SourceDetails& source_details) {
  task_runner_->PushTask(
      absl::make_unique<ReplaceTask>(scene_graph_, replace, source_details));
}

UUID RootController::AddPath(const ink::proto::Path& unsafe_path,
                             const GroupId& group,
                             const SourceDetails& source_details) {
  unique_ptr<BezierPathConverter> converter(
      new BezierPathConverter(unsafe_path));
  UUID id = scene_graph_->GenerateUUID();
  unique_ptr<SceneElementAdder> adder_task(new SceneElementAdder(
      move(converter), scene_graph_, source_details, id,
      /* below_element_with_id= */ kInvalidElementId, group));
  task_runner_->PushTask(move(adder_task));
  return id;
}

UUID RootController::AddMeshFromEngine(const Mesh& mesh,
                                       const ElementAttributes& attributes,
                                       GroupId group, UUID uuid) {
  unique_ptr<MeshConverter> converter(
      new MeshConverter(ShaderType::TexturedVertShader, mesh, attributes));
  SourceDetails source_details = SourceDetails::FromEngine();

  unique_ptr<SceneElementAdder> adder_task(new SceneElementAdder(
      std::move(converter), scene_graph_, source_details, uuid,
      /* below_element_with_id= */ kInvalidElementId, group));
  task_runner_->PushTask(move(adder_task));
  return uuid;
}

UUID RootController::AddImageRect(const Rect& rectangle, float rotation,
                                  const std::string& uri,
                                  const ElementAttributes& attributes,
                                  GroupId group_id) {
  Mesh mesh;
  MakeImageRectMesh(&mesh, rectangle, rectangle, uri);
  mesh.object_matrix =
      matrix_utils::RotateAboutPoint(rotation, rectangle.Center());
  UUID uuid = scene_graph_->GenerateUUID();
  return AddMeshFromEngine(mesh, attributes, group_id, uuid);
}

UUID RootController::AddTextRect(const text::TextSpec& text, const Rect& rect,
                                 GroupId group, UUID uuid,
                                 const glm::mat4& transform,
                                 const ElementId& below_element_with_id) {
  std::string uri = registry_->GetShared<TextTextureProvider>()->AddText(
      text, uuid,
      std::round(camera_->ConvertDistance(rect.Width(), DistanceType::kWorld,
                                          DistanceType::kScreen)),
      std::round(camera_->ConvertDistance(rect.Height(), DistanceType::kWorld,
                                          DistanceType::kScreen)));
  Mesh mesh;
  MakeImageRectMesh(&mesh, rect, rect, uri);
  mesh.object_matrix = transform;

  auto converter = absl::make_unique<TextMeshConverter>(mesh, text);
  SourceDetails source_details = SourceDetails::FromEngine();

  unique_ptr<SceneElementAdder> adder_task(
      new SceneElementAdder(std::move(converter), scene_graph_, source_details,
                            uuid, below_element_with_id, group));
  task_runner_->PushTask(move(adder_task));
  return uuid;
}

void RootController::UpdateText(const UUID& uuid, const text::TextSpec& text,
                                const Rect& world_bounds) {
  auto element_id = scene_graph_->ElementIdFromUUID(uuid);

  Rect base(0, 0, kTextBoxSize, kTextBoxSize);
  // Set the element's transform such that the text is at world_rect.
  scene_graph_->TransformElement(element_id, base.CalcTransformTo(world_bounds),
                                 SourceDetails::EngineInternal());
  // Text contents / size may have changed, new texture needed.
  registry_->GetShared<TextTextureProvider>()->UpdateText(uuid, text);
}

void RootController::UpdateText(const UUID& uuid, const text::TextSpec& text,
                                float width_multiplier,
                                float height_multiplier) {
  ElementId element_id = scene_graph_->ElementIdFromUUID(uuid);

  auto transform = scene_graph_->GetElementMetadata(element_id).world_transform;
  glm::vec2 bottom_center =
      geometry::Transform(glm::vec2(kTextBoxSize / 2.0, 0), transform);
  auto rotation = matrix_utils::RotateAboutPoint(
      matrix_utils::GetRotationComponent(transform), bottom_center);

  // Expected world size is the current size * multipliers
  auto expected_size = matrix_utils::GetScaleComponent(transform) *
                       glm::vec2({kTextBoxSize, kTextBoxSize}) *
                       glm::vec2({width_multiplier, height_multiplier});

  // Update the transform matrix such that bottom_center is at the same
  // location, rotation is preserved and width and height match expected_size.

  // Unrotated box with the proper position and scale.
  Rect unrotated(bottom_center.x - expected_size.x / 2, bottom_center.y,
                 bottom_center.x + expected_size.x / 2,
                 bottom_center.y + expected_size.y);

  Rect base(0, 0, kTextBoxSize, kTextBoxSize);
  // Rotate about the bottom center
  glm::mat4 updated_transform = rotation * base.CalcTransformTo(unrotated);

  scene_graph_->TransformElement(element_id, updated_transform,
                                 SourceDetails::EngineInternal());
  // Text contents / size may have changed, new texture needed.
  registry_->GetShared<TextTextureProvider>()->UpdateText(uuid, text);
}

void RootController::SetTransforms(const std::vector<UUID>& uuids,
                                   const std::vector<glm::mat4>& new_transforms,
                                   const SourceDetails& source_details) {
  if (uuids.size() != new_transforms.size()) {
    SLOG(SLOG_ERROR, "SetTransforms expected equal sizes. Actual: $0, $1",
         uuids.size(), new_transforms.size());
    return;
  }

  std::vector<ElementId> el_ids(uuids.size());
  for (size_t i = 0; i < uuids.size(); i++) {
    el_ids[i] = scene_graph_->ElementIdFromUUID(uuids[i]);
  }

  scene_graph_->TransformElements(el_ids.begin(), el_ids.end(),
                                  new_transforms.begin(), new_transforms.end(),
                                  source_details);
}

void RootController::SetVisibilities(const std::vector<UUID>& uuids,
                                     const std::vector<bool>& visibilities,
                                     const SourceDetails& source_details) {
  if (uuids.size() != visibilities.size()) {
    SLOG(SLOG_ERROR, "SetVisibilities expected equal sizes. Actual: $0, $1",
         uuids.size(), visibilities.size());
    return;
  }

  std::vector<ElementId> el_ids(uuids.size());
  for (size_t i = 0; i < uuids.size(); ++i) {
    el_ids[i] = scene_graph_->ElementIdFromUUID(uuids[i]);
  }

  scene_graph_->SetVisibilities(begin(el_ids), end(el_ids), begin(visibilities),
                                end(visibilities), source_details);
}

void RootController::SetOpacities(const std::vector<UUID>& uuids,
                                  const std::vector<int>& opacities,
                                  const SourceDetails& source_details) {
  if (uuids.size() != opacities.size()) {
    SLOG(SLOG_ERROR, "SetOpacities expected equal sizes. Actual: $0, $1",
         uuids.size(), opacities.size());
    return;
  }

  std::vector<ElementId> el_ids(uuids.size());
  for (size_t i = 0; i < uuids.size(); ++i) {
    el_ids[i] = scene_graph_->ElementIdFromUUID(uuids[i]);
  }

  scene_graph_->SetOpacities(begin(el_ids), end(el_ids), begin(opacities),
                             end(opacities), source_details);
}

void RootController::ChangeZOrders(const std::vector<UUID>& uuids,
                                   const std::vector<UUID>& below_uuids,
                                   const SourceDetails& source_details) {
  if (uuids.size() != below_uuids.size()) {
    SLOG(SLOG_ERROR, "ChangeZOrders expected equal sizes. Actual: $0, $1",
         uuids.size(), below_uuids.size());
    return;
  }

  std::vector<ElementId> el_ids(uuids.size());
  std::vector<ElementId> below_el_ids(uuids.size());
  for (size_t i = 0; i < uuids.size(); ++i) {
    el_ids[i] = scene_graph_->ElementIdFromUUID(uuids[i]);
    below_el_ids[i] = scene_graph_->ElementIdFromUUID(below_uuids[i]);
  }

  scene_graph_->ChangeZOrders(begin(el_ids), end(el_ids), begin(below_el_ids),
                              end(below_el_ids), source_details);
}

void RootController::SetColor(const UUID& uuid, glm::vec4 rgba,
                              SourceDetails source) {
  ElementId id = scene_graph_->ElementIdFromUUID(uuid);
  scene_graph_->SetColor(id, rgba, source);
}

void RootController::Render(uint32_t max_dimension_px,
                            bool should_draw_background, const Rect& world_rect,
                            GroupId render_only_group, ExportedImage* out) {
  Rect image_export_bounds;
  if (!world_rect.Empty()) {
    image_export_bounds = world_rect;
  } else {
    if (page_bounds_->HasBounds()) {
      image_export_bounds = page_bounds_->Bounds();
    } else {
      // Use the scene mbr if we have no page bounds and any elements.
      image_export_bounds = scene_graph_->Mbr();
    }
    if (image_export_bounds.Area() == 0) {
      // If we have no elements AND no page bounds, use the current camera.
      image_export_bounds = camera_->WorldWindow();
    }
  }

  ImageExporter::Render(max_dimension_px, image_export_bounds,
                        frame_state_->GetFrameTime(), gl_resources_,
                        page_bounds_, wall_clock_, *scene_graph_,
                        should_draw_background, render_only_group, out);
}

void RootController::AddSequencePoint(int32_t id) {
  task_runner_->PushTask(
      absl::make_unique<SequencePointTask>(id, frame_state_));
}

void RootController::DeselectAll() {
  tools::EditTool* edit_tool = nullptr;
  if (tools_->GetTool(Tools::Edit, &edit_tool)) {
    edit_tool->CancelManipulation();
  }
}

void RootController::SetSelectionProvider(
    std::shared_ptr<ISelectionProvider> provider) {
  selection_provider_ = std::move(provider);
  TextHighlighterTool* highlighter_tool = nullptr;
  if (tools_->GetTool(Tools::TextHighlighterTool, &highlighter_tool)) {
    highlighter_tool->SetSelectionProvider(selection_provider_);
  }
  SmartHighlighterTool* smart_highlighter_tool = nullptr;
  if (tools_->GetTool(Tools::SmartHighlighterTool, &smart_highlighter_tool)) {
    smart_highlighter_tool->SetSelectionProvider(selection_provider_);
  }
}

ISelectionProvider* RootController::SelectionProvider() const {
  return selection_provider_.get();
}

}  // namespace ink
