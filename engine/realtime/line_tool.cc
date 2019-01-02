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

#include "ink/engine/realtime/line_tool.h"

#include <algorithm>
#include <atomic>
#include <cstddef>  // for size_t
#include <deque>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/tool_type.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/tess/cdrefinement.h"
#include "ink/engine/geometry/tess/color_linearizer.h"
#include "ink/engine/gl.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/math_defines.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/scissor.h"
#include "ink/engine/rendering/scene_drawable.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/float_pack.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

namespace {

constexpr int kMaxMidpointsPerLine = 4000;
constexpr uint32_t kDbgPredictionId = 66;
constexpr uint32_t kDbgTriangulationId = 88;

}  // namespace

LineTool::LineTool(const service::UncheckedRegistry& registry,
                   InputRegistrationPolicy input_registration_policy)
    : Tool(),
      gl_resources_(registry.GetShared<GLResourceManager>()),
      result_sink_(registry.GetShared<LineToolDataSink>()),
      input_modeler_(registry.GetShared<input::InputModeler>()),
      line_modifier_factory_(registry.GetShared<LineModifierFactory>()),
      page_bounds_(registry.GetShared<PageBounds>()),
      page_manager_(registry.GetShared<PageManager>()),
      layer_manager_(registry.GetShared<LayerManager>()),
      dbg_helper_(registry.GetShared<IDbgHelper>()),
      has_touch_id_(false),
      touch_id_(0),
      rgba_(glm::vec4(0, 0, 0, 1)),
      renderer_(registry),
      line_builder_(registry.GetShared<settings::Flags>(),
                    registry.GetShared<GLResourceManager>()),
      particles_(registry),
      should_init_shader_metadata_(false),
      shape_feedback_(ShapeGeometry::Type::Circle),
      shape_feedback_renderer_(registry),
      input_points_(new InputPoints()),
      current_group_(kInvalidElementId) {
  shape_feedback_.SetBorderColor(glm::vec4(1, 1, 1, 1),
                                 glm::vec4(0.6f, 0.6f, 0.6f, 1));
  shape_feedback_.SetFillVisible(false);

  if (input_registration_policy == InputRegistrationPolicy::ACTIVE) {
    RegisterForInput(registry.GetShared<input::InputDispatch>());
  }
  Clear();
}

void LineTool::SetBrushParams(BrushParams params) {
  // If the line type changed, check whether we need to preload textures.
  //
  // We preload textures for texture brushes as soon as the user selects the
  // tool, so that the textures are hopefully loaded before the user starts
  // drawing. It's not an *error* if the textures haven't loaded first, but the
  // user's line won't look right until textures are done loading.
  if (brush_params_.line_modifier != params.line_modifier) {
    LoadBrushTextures(params);
  }
  brush_params_ = params;
  input_modeler_->SetParams(params.shape_params,
                            params.size.WorldSize(input_modeler_->camera()));
}

void LineTool::SetColor(glm::vec4 rgba) { rgba_ = rgba; }

void LineTool::Update(const Camera& camera, FrameTimeS draw_time) {
  if (should_init_shader_metadata_) {
    ShaderMetadata metadata = ShaderMetadata::Default();
    if (brush_params_.animated ||
        brush_params_.line_modifier == BrushParams::LineModifier::HIGHLIGHTER) {
      metadata = ShaderMetadata::Animated(draw_time);
    } else if (brush_params_.line_modifier ==
               BrushParams::LineModifier::ERASER) {
      metadata = ShaderMetadata::Eraser();
    }
    line_builder_.SetShaderMetadata(metadata);

    if (brush_params_.particles) {
      particles_.SetInitTime(draw_time);
    }

    should_init_shader_metadata_ = false;
  }
}

void LineTool::Draw(const Camera& live_camera, FrameTimeS draw_time) const {
  std::unique_ptr<Scissor> scissor;
  // Capture the previous scissor and use the scissor of the currect page if one
  // is defined. The group may not exist if for some reason the page got removed
  // while the tip is down.
  if (page_manager_->MultiPageEnabled() &&
      current_group_ != kInvalidElementId &&
      page_manager_->GroupExists(current_group_)) {
    // Pages are assumed to be clippable groups.
    auto page = page_manager_->GetPageInfo(current_group_);
    scissor = absl::make_unique<Scissor>(gl_resources_->gl);
    scissor->SetScissor(live_camera, page.bounds, CoordType::kWorld);
  }

  renderer_.Draw(live_camera, draw_time, line_builder_.StableMesh());
  renderer_.Draw(live_camera, draw_time, line_builder_.UnstableMesh());
  renderer_.Draw(live_camera, draw_time, line_builder_.PredictionMesh());

  shape_feedback_renderer_.Draw(live_camera, draw_time, shape_feedback_);

  DebugDrawTriangulation();
}

void LineTool::UpdateShapeFeedback(const input::InputData& data,
                                   float world_radius,
                                   const Camera& live_camera) {
  if (data.Get(input::Flag::TUp) || !brush_params_.show_input_feedback) {
    shape_feedback_.SetVisible(false);
    return;
  }
  shape_feedback_.SetVisible(true);
  glm::vec2 border_size_world{
      live_camera.ConvertDistance(1, DistanceType::kDp, DistanceType::kWorld)};
  auto location =
      Rect::CreateAtPoint(data.world_pos, 2.0f * glm::vec2(world_radius));
  shape_feedback_.SetSizeAndPosition(location, border_size_world, true);
}

void LineTool::Enable(bool enabled) {
  if (!enabled) Clear();
  Tool::Enable(enabled);
}

void LineTool::Clear() {
  sent_up_ = false;
  has_touch_id_ = false;
  line_builder_.Clear();
  if (brush_params_.particles) {
    particles_.Clear();
  }
  shape_feedback_.SetVisible(false);
  // We make a new instance instead of calling Clear() because we might have
  // std::move'd it into the data sink.
  input_points_ = absl::make_unique<InputPoints>();
  should_init_shader_metadata_ = false;
  input_region_ = Rect();
  current_group_ = kInvalidElementId;
}

void LineTool::EnableDebugMesh(bool enabled) {
  dbg_mesh_enabled_ = enabled;
  if (!enabled) {
    dbg_helper_->Remove(kDbgTriangulationId);
    dbg_helper_->Remove(kDbgPredictionId);
  }
}

void LineTool::SetupNewLine(const input::InputData& data,
                            const Camera& live_camera) {
  auto p = brush_params_;

  Clear();

  if (p.particles) {
    particles_.SetupNewLine(rgba_);
  }

  auto line_modifier = line_modifier_factory_->Make(brush_params_, rgba_);
  line_modifier->SetupNewLine(data, live_camera);

  auto predicted_modifier = line_modifier_factory_->Make(brush_params_, rgba_);
  predicted_modifier->SetupNewLine(data, live_camera);

  has_touch_id_ = true;
  touch_id_ = data.id;

  final_tessellation_params_ =
      ConvertLineModParamsToTessellationParams(line_modifier->params());

  input_modeler_->Reset(live_camera, input::InputModelParams(data.type));
  input_modeler_->SetParams(p.shape_params, p.size.WorldSize(live_camera));

  should_init_shader_metadata_ = true;

  line_builder_.SetupNewLine(live_camera, brush_params_.tip_type, data.time,
                             data.type, std::move(line_modifier),
                             std::move(predicted_modifier));
  input_region_ = Rect(data.world_pos, data.world_pos);

  // If both page manager and layers are enabled, page manager wins.
  if (page_manager_->MultiPageEnabled()) {
    current_group_ = page_manager_->GetPageGroupForRect(input_region_);
  } else {
    size_t index;
    GroupId group_id;
    if (layer_manager_->IndexOfActiveLayer(&index) &&
        layer_manager_->GroupIdForLayerAtIndex(index, &group_id)) {
      current_group_ = group_id;
    }
  }

  if (dbg_mesh_enabled_) {
    dbg_helper_->Remove(kDbgTriangulationId);
    dbg_helper_->Remove(kDbgPredictionId);
  }
}

TessellationParams LineTool::ConvertLineModParamsToTessellationParams(
    LineModParams line_mod_params) {
  TessellationParams result;
  result.linearize_mesh_verts = line_mod_params.linearize_mesh_verts;
  result.linearize_combined_verts = line_mod_params.linearize_combined_verts;
  result.texture_uri = line_mod_params.texture_uri;
  result.use_endcaps_on_all_lines = false;
  return result;
}

void LineTool::LoadBrushTextures(const BrushParams params) {
  auto line_mod_params = line_modifier_factory_->Make(params, rgba_)->params();
  if (line_mod_params.texture_uri.empty()) {
    return;
  }
  Texture* texture = nullptr;
  TextureInfo texture_info(line_mod_params.texture_uri);
  gl_resources_->texture_manager->GetTexture(texture_info, &texture);
}

bool LineTool::ShouldClearAndRefuseInput(const input::InputData& data) const {
  if (sent_up_) {
    // We've already cut the line short because it was so long.
    return true;
  }

  if (data.Get(input::Flag::Cancel) || data.Get(input::Flag::Right)) {
    // We received an explicit cancel, or a right click.
    return true;
  }

  if (page_manager_->MultiPageEnabled() &&
      current_group_ == kInvalidElementId) {
    // Pagination is enabled and the input started outside of the page.
    // Toss it!
    return true;
  }

  return false;
}

void LineTool::ChangeToTUp(input::InputData* data) {
  EXPECT(!data->Get(input::Flag::TDown));
  data->Set(input::Flag::TUp, true);
  data->Set(input::Flag::InContact, false);
  sent_up_ = true;
}

bool LineTool::IsDrawing() const { return has_touch_id_; }

Rect LineTool::InputRegion() const { return input_region_; }

input::CaptureResult LineTool::OnInput(const input::InputData& in_data,
                                       const Camera& live_camera) {
  input::InputData data = in_data;
  if (data.Get(input::Flag::Primary) && data.Get(input::Flag::TDown)) {
    SetupNewLine(data, live_camera);
  }

  if (ShouldClearAndRefuseInput(data)) {
    Clear();
    return input::CapResRefuse;
  }

  if (line_builder_.MidPointCount() > kMaxMidpointsPerLine) ChangeToTUp(&data);

  // Draw lines with one contact only (don't clear, the main line should
  // continue being extruded).
  if (!has_touch_id_ || touch_id_ != data.id) {
    return input::CapResObserve;
  }

  // This should never happen: if we have an active stream and observe something
  // that is InContact or a TUp then it is either a violation of the input
  // stream semantics or a programmer error in LineTool.
  if (!data.Get(input::Flag::InContact) && !data.Get(input::Flag::TUp)) {
    ASSERT(false);
    Clear();
    return input::CapResRefuse;
  }

  input_region_ = input_region_.Join(data.world_pos);

  bool is_line_end = data.Get(input::Flag::TUp);

  ExtrudeLine(data, is_line_end);

  if (is_line_end) {
    SendCompleteLineToSink();
    Clear();
  } else {
    RegeneratePredictedLine();

    float world_radius =
        brush_params_.shape_params
            .GetRadius(brush_params_.size.WorldSize(input_modeler_->camera()))
            .x;
    UpdateShapeFeedback(data, world_radius, live_camera);
  }

  return input::CapResCapture;
}

void LineTool::ExtrudeLine(const input::InputData& data, bool is_line_end) {
  input_modeler_->AddInputToModel(data);
  input_points_->AddRawInputPoint(data.world_pos, data.time);

  std::vector<input::ModeledInput> model_results;
  while (input_modeler_->HasModelResult()) {
    input::ModeledInput mi = input_modeler_->PopNextModelResult();
    input_points_->AddModeledInputPoint(mi.world_pos, mi.time,
                                        mi.tip_size.radius);
    model_results.emplace_back(mi);
  }
  if (!model_results.empty()) {
    line_builder_.ExtrudeModeledInput(input_modeler_->camera(), model_results,
                                      is_line_end);
    if (brush_params_.particles) {
      particles_.ExtrudeModeledInput(model_results);
    }
  }
}

void LineTool::RegeneratePredictedLine() {
  DebugDrawPrediction();

  auto points = input_modeler_->PredictModelResults();
  auto& cam = input_modeler_->camera();
  line_builder_.ConstructPrediction(cam, points);
}

void LineTool::SendCompleteLineToSink() {
  if (line_builder_.MidPointCount() == 0) return;

  auto rendering_mesh = absl::make_unique<Mesh>(line_builder_.StableMesh());

  // If the mesh is entirely off the page, throw it away (these are mostly just
  // accidental touches off the page that pollute the undo stack and hwr data).
  // Otherwise send it on to the result_sink.
  Rect obj_coord_mbr = geometry::Envelope(rendering_mesh->verts);
  Rect mbr = geometry::Transform(obj_coord_mbr, rendering_mesh->object_matrix);
  const bool in_bounds = !page_bounds_->HasBounds() ||
                         geometry::Intersects(page_bounds_->Bounds(), mbr);
  const bool in_page =
      !page_manager_->MultiPageEnabled() || current_group_ != kInvalidElementId;
  SLOG(SLOG_DATA_FLOW, "New line tests: in_bounds=$0 in_page=$1", in_bounds,
       in_page);
  if (in_bounds && in_page) {
    result_sink_->Accept(line_builder_.CompletedLines(),
                         std::move(input_points_), std::move(rendering_mesh),
                         current_group_,
                         line_builder_.GetLineModifier()->GetShaderType(),
                         final_tessellation_params_);
  }
}

void LineTool::DebugDrawTriangulation() const {
  if (dbg_mesh_enabled_) {
    dbg_helper_->Remove(kDbgTriangulationId);
    dbg_helper_->AddMeshSkeleton(line_builder_.StableMesh(), {0, 0, 1, .6},
                                 {1, 0, 1, .6}, kDbgTriangulationId);
    dbg_helper_->AddMeshSkeleton(line_builder_.UnstableMesh(), {0, 1, 1, .6},
                                 {1, 0, 1, .6}, kDbgTriangulationId);
    dbg_helper_->AddMeshSkeleton(line_builder_.PredictionMesh(), {0, 1, 0, .6},
                                 {1, 0, 1, .6}, kDbgTriangulationId);
  }
}

void LineTool::DebugDrawPrediction() {
  if (dbg_helper_->PredictedLineVisualizationEnabled()) {
    auto& cam = input_modeler_->camera();
    prediction_dbg_count_++;
    // Draw old mesh before we erase it and make a new one.
    const auto& dbg_mesh = line_builder_.PredictionMesh();
    if (!dbg_mesh.verts.empty() && prediction_dbg_count_ % 20 == 0) {
      prediction_dbg_color_.a = 0.3;
      prediction_dbg_color_.r = Fract(prediction_dbg_color_.r + 0.4);
      prediction_dbg_color_.g = Fract(prediction_dbg_color_.g + 0.4);
      prediction_dbg_color_.b = Fract(prediction_dbg_color_.b + 0.4);
      dbg_helper_->AddMesh(dbg_mesh, prediction_dbg_color_, kDbgPredictionId);
      for (const auto& midpt : line_builder_.PredictionMidPoints()) {
        Vertex vx(cam.ConvertPosition(midpt.screen_position, CoordType::kScreen,
                                      CoordType::kWorld));
        prediction_dbg_color_.a = 1.0;
        vx.color = prediction_dbg_color_;
        dbg_helper_->AddPoint(vx, 2.0f, kDbgPredictionId);
      }
    }
  }
}

}  // namespace ink
