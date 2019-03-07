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

#include "ink/engine/public/sengine.h"
#include <memory>

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/match.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/input/sinput.h"
#include "ink/engine/input/sinput_helpers.h"
#include "ink/engine/processing/blocker_manager.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/realtime/edit_tool.h"
#include "ink/engine/rendering/compositing/live_renderer.h"
#include "ink/engine/rendering/gl_managers/text_texture_provider.h"
#include "ink/engine/rendering/strategy/rendering_strategy.h"
#include "ink/engine/scene/default_services.h"
#include "ink/engine/scene/element_animation/element_animation.h"
#include "ink/engine/scene/element_animation/element_animation_controller.h"
#include "ink/engine/scene/graph/scene_change_notifier.h"
#include "ink/engine/scene/grid_assets/grid_texture_provider.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/page/horizontal_page_layout.h"
#include "ink/engine/scene/page/page_info.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/scene/page/vertical_page_layout.h"
#include "ink/engine/scene/types/element_bundle.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/util/dbg/glerrors.h"
#include "ink/engine/util/dbg_helper.h"
#include "ink/engine/util/funcs/rand_funcs.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/public/document/single_user_document.h"
#include "ink/public/document/storage/in_memory_storage.h"

namespace ink {

using util::ReadFromProto;

static const float kMaxCameraConfigMarginPx = 10000;
static const float kMaxCameraConfigPadding = 1;

SEngine::SEngine(std::shared_ptr<IHost> host, const proto::Viewport& viewport,
                 uint64_t random_seed)
    : SEngine(host, viewport, random_seed,
              std::make_shared<SingleUserDocument>(
                  std::make_shared<InMemoryStorage>())) {}

SEngine::SEngine(std::shared_ptr<IHost> host, const proto::Viewport& viewport,
                 uint64_t random_seed, std::shared_ptr<Document> document)
    : SEngine(host, viewport, random_seed, document,
              DefaultServiceDefinitions()) {}

SEngine::SEngine(std::shared_ptr<IHost> host, const proto::Viewport& viewport,
                 uint64_t random_seed, std::shared_ptr<Document> document,
                 std::unique_ptr<service::DefinitionList> service_definitions)
    : host_(host) {
  // Initialize the RootController explicitly in the constructor instead of in
  // the InitializerList to guarantee that seed_random() is called before any
  // potential rand() calls.
  Seed_random(random_seed);

  root_controller_ =
      absl::make_unique<RootController>(host, std::move(service_definitions));

  root_controller_->service<PublicEvents>()->AddElementListener(host.get());
  root_controller_->service<PublicEvents>()->AddEngineListener(host.get());
  root_controller_->service<PublicEvents>()->AddMutationListener(host.get());
  root_controller_->service<PublicEvents>()->AddPagePropertiesListener(
      host.get());
  root_controller_->service<PublicEvents>()->AddSceneChangeListener(host.get());

  scene_change_notifier_ = std::make_shared<SceneChangeNotifier>(
      root_controller_->service<PublicEvents>(),
      root_controller_->service<LayerManager>());

  setViewport(viewport);

  SetDocument(document);

  AddTextureRequestHandler("text",
                           root_controller_->service<TextTextureProvider>());
  AddTextureRequestHandler("grids", std::make_shared<GridTextureProvider>());

  input_receiver_ = root_controller_->service<input::InputReceiver>();

  root_controller_->service<LayerManager>()->AddActiveLayerListener(
      scene_change_notifier_.get());

  root_controller_->service<SceneGraph>()->AddListener(
      scene_change_notifier_.get());

  if (host->ShouldPreloadShaders()) {
    root_controller_->service<GLResourceManager>()
        ->shader_manager->LoadAllShaders();
  }
}

void SEngine::UndoRedoStateChanged(bool canUndo, bool canRedo) {
  host_->UndoRedoStateChanged(canUndo, canRedo);
}

void SEngine::SetDocument(std::shared_ptr<Document> document) {
  SLOG(SLOG_INFO, "setting document to $0", *document);

  if (document == document_) {
    SLOG(SLOG_INFO, "ignoring request to set document to current document");
    return;
  }

  scene_change_notifier_->SetEnabled(false);
  root_controller_->service<SceneGraph>()->SetBulkLoading(true);

  auto layer_manager = registry()->GetShared<LayerManager>();

  if (document_) {
    // Unregister the existing listeners and clear the scene.
    layer_manager->RemoveActiveLayerListener(document_.get());
    root_controller_->service<PublicEvents>()->RemoveElementListener(
        document_.get());
    root_controller_->service<PublicEvents>()->RemovePagePropertiesListener(
        document_.get());
    document_->RemoveElementListener(root_controller_->unsafe_helper_.get());
    document_->RemovePagePropertiesListener(
        root_controller_->unsafe_helper_.get());
    document_->RemoveDocumentListener(this);
    document_->RemoveMutationListener(host_.get());
    document_->RemoveActiveLayerListener(layer_manager.get());
    clear();
  }

  // Tell the document that this is the GL thread.
  document->SetPreferredThread();

  // Register new document listeners
  document_ = document;

  root_controller_->service<PublicEvents>()->AddElementListener(
      document_.get());
  root_controller_->service<PublicEvents>()->AddPagePropertiesListener(
      document_.get());

  layer_manager->AddActiveLayerListener(document_.get());

  // Use the scene's unsafe_helper to send events from document directly
  // to the scene controller.
  document_->AddElementListener(root_controller_->unsafe_helper_.get());
  document_->AddPagePropertiesListener(root_controller_->unsafe_helper_.get());
  document_->AddDocumentListener(this);
  document_->AddMutationListener(host_.get());
  document_->AddActiveLayerListener(layer_manager.get());

  // Load the document into the scene graph.
  if (document_->SupportsQuerying()) {
    proto::SourceDetails host_source;
    host_source.set_origin(proto::SourceDetails::HOST);
    auto pp = document_->GetPageProperties();
    if (pp.has_bounds()) {
      root_controller_->unsafe_helper_->PageBoundsChanged(pp.bounds(),
                                                          host_source);
    }
    if (pp.has_border()) {
      root_controller_->unsafe_helper_->BorderChanged(pp.border(), host_source);
    }

    if (pp.has_background_color()) {
      root_controller_->unsafe_helper_->BackgroundColorChanged(
          pp.background_color(), host_source);
    } else if (pp.has_background_image()) {
      root_controller_->unsafe_helper_->BackgroundImageChanged(
          pp.background_image(), host_source);
    } else {
      // Always reset to default background color if no background is specified.
      auto glr = root_controller_->service<GLResourceManager>();
      glr->background_state->SetToDefaultColor(glr->texture_manager.get());
    }

    if (pp.has_grid_info()) {
      root_controller_->unsafe_helper_->GridChanged(pp.grid_info(),
                                                    host_source);
    }
    auto snapshot = document_->GetSnapshot(
        Document::SnapshotQuery::DO_NOT_INCLUDE_UNDO_STACK);

    // First add all groups, then all elements.
    for (const auto& b : snapshot.element()) {
      if (b.element().attributes().is_group()) {
        SLOG(SLOG_DOCUMENT, "loading group $0", b.uuid());
        root_controller_->unsafe_helper_
            ->AddElement(b, kInvalidUUID, host_source)
            .IgnoreError();
      }
    }
    for (const auto& b : snapshot.element()) {
      if (!b.element().attributes().is_group()) {
        SLOG(SLOG_DOCUMENT, "loading element $0 as child of $1", b.uuid(),
             b.group_uuid());
        root_controller_->unsafe_helper_
            ->AddElement(b, kInvalidUUID, host_source)
            .IgnoreError();
      }
    }

    // If we have an active layer stored in the document, set it active in
    // LayerManager. If we don't have an element with that ID, we fall back to
    // SetActiveLayer(0), because that's better than leaving it uninitialized.
    if (snapshot.has_active_layer_uuid() &&
        UUID(snapshot.active_layer_uuid()) != kInvalidUUID) {
      GroupId group_id =
          root_controller_->service<SceneGraph>()->GroupIdFromUUID(
              snapshot.active_layer_uuid());
      auto layer_index_or = layer_manager->IndexForLayerWithGroupId(group_id);
      if (group_id != kInvalidElementId && layer_index_or.ok()) {
        SLOG(SLOG_DOCUMENT, "setting active layer to $0 (layer index $1)",
             group_id, layer_index_or.ValueOrDie());
        // This SetActiveLayer comes from the host; the user should not be
        // allowed to undo it.
        layer_manager->SetActiveLayer(layer_index_or.ValueOrDie(),
                                      SourceDetails::FromHost(0));
      } else if (layer_manager->NumLayers() > 0) {
        SLOG(SLOG_WARNING, "layer UUID $0 not found; setting layer 0 active",
             snapshot.active_layer_uuid());
        layer_manager->SetActiveLayer(0, SourceDetails::FromHost(0));
      } else {
        SLOG(SLOG_ERROR, "active layer specified, but no layers available");
      }
    }

    // Add pages. Groups should already have been added to the scene.
    if (!snapshot.per_page_properties().empty()) {
      auto per_page_properties = snapshot.per_page_properties();
      auto page_manager = registry()->GetShared<PageManager>();
      for (auto& page : per_page_properties) {
        if (!page_manager->AddPageFromProto(page)) {
          SLOG(SLOG_ERROR, "Could not add page: $0", page.uuid());
        }
      }
      page_manager->GenerateLayout();
    }
  }

  // Don't start notifying about adds until all just-loaded elements make their
  // way through the add queue.
  root_controller_->service<ITaskRunner>()->PushTask(
      absl::make_unique<FlushTask>([&]() {
        SLOG(SLOG_DATA_FLOW, "Re-enabling scene change notifier.");
        scene_change_notifier_->SetEnabled(true);
        root_controller_->service<SceneGraph>()->SetBulkLoading(false);
      }));

  // Dispatch the current undo/redo state for this document.
  document_->NotifyUndoRedoStateChanged(document_->CanUndo(),
                                        document_->CanRedo());
  // Likewise for the empty state.
  document_->NotifyEmptyStateChanged(document_->IsEmpty());

  root_controller_->service<LiveRenderer>()->Invalidate();
}

void SEngine::SetRenderingStrategy(
    proto::RenderingStrategy rendering_strategy) {
  ink::RenderingStrategy strategy;
  if (!util::ReadFromProto(rendering_strategy, &strategy)) {
    SLOG(SLOG_ERROR, "Cannot read rendering strategy from given proto.");
    return;
  }
  SetRenderingStrategy(strategy);
}

void SEngine::SetRenderingStrategy(RenderingStrategy rendering_strategy) {
  registry()->Get<LiveRenderer>()->Use(rendering_strategy);
}

void SEngine::SetPageLayout(SEngine::PageLayout strategy, float spacing_world) {
  if (CheckBlockedState()) return;

  auto page_manager = root_controller_->service<PageManager>();
  std::unique_ptr<ink::PageLayoutStrategy> layout;
  switch (strategy) {
    case PageLayout::VERTICAL: {
      auto vertical = absl::make_unique<VerticalPageLayout>();
      vertical->SetSpacingWorld(spacing_world);
      layout = std::move(vertical);
    } break;
    case PageLayout::HORIZONTAL: {
      auto horizontal = absl::make_unique<HorizontalPageLayout>();
      horizontal->SetSpacingWorld(spacing_world);
      layout = std::move(horizontal);
    } break;
  }
  page_manager->SetLayoutStrategy(std::move(layout));
  page_manager->GenerateLayout();

  ink::proto::Rect bounds;
  ink::util::WriteToProto(&bounds, page_manager->GetFullBounds());
  if (!document()->SetPageBounds(bounds)) {
    SLOG(SLOG_ERROR, "invalid bounds provided by page manager");
  }
}

void SEngine::SetPageLayout(proto::LayoutSpec spec) {
  PageLayout layout;
  switch (spec.strategy()) {
    case proto::LayoutStrategy::VERTICAL:
      layout = PageLayout::VERTICAL;
      break;
    case proto::LayoutStrategy::HORIZONTAL:
      layout = PageLayout::HORIZONTAL;
      break;
    default:
      RUNTIME_ERROR("unknown strategy $0",
                    static_cast<uint32_t>(spec.strategy()));
  }
  SetPageLayout(layout, spec.spacing_world());
}

float SEngine::GetPageSpacingWorld() const {
  auto page_manager = root_controller_->service<PageManager>();
  if (!page_manager->MultiPageEnabled()) return 0;
  if (auto linear_strat = dynamic_cast<LinearLayoutStrategy*>(
          page_manager->GetLayoutStrategy())) {
    return linear_strat->GetSpacingWorld();
  }
  return 0;
}

proto::Rects SEngine::GetPageLocations() const {
  proto::Rects result;
  auto page_manager = root_controller_->service<PageManager>();
  if (page_manager->MultiPageEnabled()) {
    for (int i = 0, n = page_manager->GetNumPages(); i < n; i++) {
      util::WriteToProto(result.add_rect(),
                         page_manager->GetPageInfo(i).bounds);
    }
  }
  return result;
}

void SEngine::FocusOnPage(int page) {
  auto page_manager = root_controller_->service<PageManager>();
  if (page < 0 || page >= page_manager->GetNumPages()) {
    SLOG(SLOG_ERROR, "Requesting an out-of-bounds page: $0", page);
    return;
  }
  if (page_manager->IsDirty()) {
    SLOG(SLOG_ERROR,
         "Page Manager is dirty. Did you forget to call GenerateLayout?");
    return;
  }
  auto page_def = page_manager->GetPageInfo(page);
  static constexpr float kPdfViewMarginWorld = 10;
  Rect camera_bounds = page_def.bounds.Inset(
      glm::vec2(-kPdfViewMarginWorld, -kPdfViewMarginWorld));
  SetCameraPosition(camera_bounds);
}

void SEngine::draw() {
  WallTimeS current_time = registry()->Get<WallClockInterface>()->CurrentTime();
  draw(static_cast<double>(current_time));
}

void SEngine::draw(double draw_time) {
#if WEAR_HANDWRITING
  auto dispatch = root_controller_->service<input::InputDispatch>();
  auto cam = root_controller_->service<Camera>();
  input_receiver_->GetCoalescer()->DispatchQueuedInput(&(*dispatch), *cam);
#endif
  root_controller_->Draw(FrameTimeS(draw_time));
  GLASSERT_NO_ERROR(root_controller_->service<GLResourceManager>()->gl);
}

class UndoRedoTask : public Task {
 public:
  enum class Operation { kUndo, kRedo };

  UndoRedoTask(Operation operation, std::weak_ptr<Document> weak_document,
               std::unique_ptr<BlockerLock> lock)
      : operation_(operation),
        weak_document_(std::move(weak_document)),
        lock_(std::move(lock)) {}

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}
  void Execute() override {}
  void OnPostExecute() override {
    if (auto document = weak_document_.lock()) {
      switch (operation_) {
        case Operation::kUndo:
          if (document->CanUndo()) document->Undo();
          break;
        case Operation::kRedo:
          if (document->CanRedo()) document->Redo();
      }
    }
  }

 private:
  Operation operation_;
  std::weak_ptr<Document> weak_document_;
  std::unique_ptr<BlockerLock> lock_;
};

void SEngine::Undo() {
  if (registry()->Get<input::InputDispatch>()->GetNContacts() > 0) {
    SLOG(SLOG_WARNING, "Undo skipped due to active inputs.");
    return;
  }

  auto* task_runner = registry()->Get<ITaskRunner>();
  if (task_runner->NumPendingTasks() > 0) {
    task_runner->PushTask(absl::make_unique<UndoRedoTask>(
        UndoRedoTask::Operation::kUndo, document_,
        registry()->Get<BlockerManager>()->AcquireLock()));
  } else {
    document()->Undo();
  }
}

void SEngine::Redo() {
  if (registry()->Get<input::InputDispatch>()->GetNContacts() > 0) {
    SLOG(SLOG_WARNING, "Redo skipped due to active inputs.");
    return;
  }

  auto* task_runner = registry()->Get<ITaskRunner>();
  if (task_runner->NumPendingTasks() > 0) {
    task_runner->PushTask(absl::make_unique<UndoRedoTask>(
        UndoRedoTask::Operation::kRedo, document_,
        registry()->Get<BlockerManager>()->AcquireLock()));
  } else {
    document()->Redo();
  }
}

void SEngine::clear() {
  if (CheckBlockedState()) return;

  root_controller_->service<LayerManager>()->Reset();
  root_controller_->service<IDbgHelper>()->Clear();
  root_controller_->service<PageManager>()->Clear();
  root_controller_->service<SceneGraph>()->RemoveAllElements(
      SourceDetails::FromHost(0));
  auto cam = root_controller_->service<Camera>();
  auto dispatch = root_controller_->service<input::InputDispatch>();
  dispatch->ForceAllUp(*cam);
}

void SEngine::RemoveAllElements() {
  if (CheckBlockedState()) return;

  root_controller_->service<SceneGraph>()->RemoveAllSelectableElements();
}

void SEngine::RemoveSelectedElements() {
  if (CheckBlockedState()) return;

  auto tools = root_controller_->service<ToolController>();
  tools::EditTool* edit_tool = nullptr;
  if (!tools->GetTool(Tools::Edit, &edit_tool) ||
      !edit_tool->IsManipulating()) {
    return;
  }
  std::vector<ElementId> elements = edit_tool->Manipulation().GetElements();
  edit_tool->CancelManipulation();
  root_controller_->service<SceneGraph>()->RemoveElements(
      elements.begin(), elements.end(), SourceDetails::FromEngine());
}

void SEngine::RemoveElement(const UUID& uuid) {
  if (CheckBlockedState()) return;

  auto scene_graph = root_controller_->service<SceneGraph>();
  ElementId element_id = scene_graph->ElementIdFromUUID(uuid);
  if (element_id != kInvalidElementId) {
    scene_graph->RemoveElement(element_id, SourceDetails::FromEngine());
  }
}

void SEngine::dispatchInput(input::InputType type, uint32_t id, uint32_t flags,
                            double time, float screen_pos_x,
                            float screen_pos_y) {
  if (CheckBlockedState()) return;

  input_receiver_->DispatchInput(type, id, flags, time, screen_pos_x,
                                 screen_pos_y);
}

void SEngine::dispatchInput(input::InputType type, uint32_t id, uint32_t flags,
                            double time, float screen_pos_x, float screen_pos_y,
                            float wheel_delta_x, float wheel_delta_y,
                            float pressure, float tilt, float orientation) {
  if (CheckBlockedState()) return;

  input_receiver_->DispatchInput(type, id, flags, time, screen_pos_x,
                                 screen_pos_y, wheel_delta_x, wheel_delta_y,
                                 pressure, tilt, orientation);
}

void SEngine::dispatchInput(proto::SInputStream inputStream) {
  if (CheckBlockedState()) return;

  input_receiver_->DispatchInput(inputStream);
}

void SEngine::dispatchInput(const proto::PlaybackStream& unsafe_playback_stream,
                            bool force_camera) {
  if (CheckBlockedState()) return;

  input_receiver_->DispatchInput(unsafe_playback_stream, force_camera);
}

void SEngine::handleCommand(const proto::Command& command) {
  bool handled = false;
  if (command.has_add_path()) {
    addPath(command.add_path());
    handled = true;
  }
  if (command.has_tool_params()) {
    setToolParams(command.tool_params());
    handled = true;
  }
  if (command.has_set_viewport()) {
    setViewport(command.set_viewport());
    handled = true;
  }
  if (command.has_camera_position()) {
    SetCameraPosition(command.camera_position());
    handled = true;
  }
  if (command.has_page_bounds()) {
    if (CheckBlockedState()) return;

    document_->SetPageBounds(command.page_bounds()).IgnoreError();
    handled = true;
  }
  if (command.has_image_export()) {
    startImageExport(command.image_export());
    handled = true;
  }
  if (command.has_flag_assignment()) {
    auto& assignment = command.flag_assignment();
    assignFlag(assignment.flag(), assignment.bool_value());
    handled = true;
  }
  if (command.has_set_element_transforms()) {
    if (CheckBlockedState()) return;

    document_->ApplyMutations(command.set_element_transforms()).IgnoreError();
    handled = true;
  }
  if (command.has_add_element()) {
    if (CheckBlockedState()) return;

    const auto& add_element_command = command.add_element();
    if (!add_element_command.has_bundle()) {
      SLOG(SLOG_ERROR, "cannot add element without bundle");
      return;
    }
    if (add_element_command.has_below_element_with_uuid()) {
      document_
          ->AddBelow(add_element_command.bundle(),
                     add_element_command.below_element_with_uuid())
          .IgnoreError();
    } else {
      document_->Add(add_element_command.bundle()).IgnoreError();
    }
    handled = true;
  }
  if (command.has_background_image()) {
    if (CheckBlockedState()) return;

    document_->SetBackgroundImage(command.background_image()).IgnoreError();
    handled = true;
  }
  if (command.has_background_color()) {
    if (CheckBlockedState()) return;

    document_->SetBackgroundColor(command.background_color()).IgnoreError();
    handled = true;
  }
  if (command.has_set_out_of_bounds_color()) {
    setOutOfBoundsColor(command.set_out_of_bounds_color());
    handled = true;
  }
  if (command.has_send_input_stream()) {
    dispatchInput(command.send_input_stream());
    handled = true;
  }
  if (command.has_sequence_point()) {
    addSequencePoint(command.sequence_point());
    handled = true;
  }
  if (command.has_set_page_border()) {
    document_->SetPageBorder(command.set_page_border()).IgnoreError();
    handled = true;
  }
  if (command.has_set_camera_bounds_config()) {
    setCameraBoundsConfig(command.set_camera_bounds_config());
    handled = true;
  }
  if (command.has_deselect_all()) {
    deselectAll();
    handled = true;
  }
  if (command.has_add_image_rect()) {
    addImageRect(command.add_image_rect());
    handled = true;
  }
  if (command.has_set_callback_flags()) {
    setCallbackFlags(command.set_callback_flags());
    handled = true;
  }
  if (command.has_clear()) {
    clear();
    handled = true;
  }
  if (command.has_remove_all_elements()) {
    if (CheckBlockedState()) return;

    document_->RemoveAll().IgnoreError();
    handled = true;
  }
  if (command.has_undo()) {
    Undo();
    handled = true;
  }
  if (command.has_redo()) {
    Redo();
    handled = true;
  }
  if (command.has_evict_image_data()) {
    evictImageData(command.evict_image_data().uri());
    handled = true;
  }
  if (command.has_remove_elements()) {
    handleRemoveElementsCommand(command.remove_elements());
    handled = true;
  }
  if (command.has_commit_crop()) {
    CommitCrop();
    handled = true;
  }
  if (command.has_set_crop()) {
    SetCrop(command.set_crop());
    handled = true;
  }
  if (command.has_element_animation()) {
    handleElementAnimation(command.element_animation());
    handled = true;
  }
  if (command.has_set_grid()) {
    setGrid(command.set_grid());
    handled = true;
  }
  if (command.has_clear_grid()) {
    clearGrid();
    handled = true;
  }
  if (command.has_add_text()) {
    addText(command.add_text());
    handled = true;
  }
  if (command.has_update_text()) {
    updateText(command.update_text());
    handled = true;
  }
  if (command.has_begin_text_editing()) {
    beginTextEditing(command.begin_text_editing().uuid());
    handled = true;
  }
  if (command.has_set_mouse_wheel_behavior()) {
    SetMouseWheelBehavior(command.set_mouse_wheel_behavior().behavior());
    handled = true;
  }
  if (command.has_set_rendering_strategy()) {
    SetRenderingStrategy(command.set_rendering_strategy());
    handled = true;
  }
  if (!handled) {
    SLOG(SLOG_ERROR, "unhandled command");
  }
}

proto::ElementBundle SEngine::convertPathToBundle(
    const proto::Path& unsafe_path) {
  proto::Element path_element;
  *path_element.mutable_path() = unsafe_path;
  proto::AffineTransform path_transform;
  util::WriteToProto(&path_transform, glm::mat4(1));
  proto::ElementBundle path_bundle;

  ElementBundle::WriteToProto(
      &path_bundle, root_controller_->service<SceneGraph>()->GenerateUUID(),
      path_element, path_transform);
  return path_bundle;
}

UUID SEngine::addPath(const proto::AddPath& unsafe_add_path) {
  if (CheckBlockedState()) return kInvalidUUID;

  auto bundle = convertPathToBundle(unsafe_add_path.path());
  if (unsafe_add_path.has_uuid()) {
    bundle.set_uuid(unsafe_add_path.uuid());
  }
  if (unsafe_add_path.has_group_uuid()) {
    bundle.set_group_uuid(unsafe_add_path.group_uuid());
  }
  document_->Add(bundle).IgnoreError();
  return bundle.uuid();
}

Status SEngine::Add(proto::ElementBundle element,
                    absl::string_view below_element_uuid) {
  proto::SourceDetails sd;
  sd.set_origin(proto::SourceDetails::ENGINE);
  return root_controller_->unsafe_helper_->AddElement(
      element, static_cast<UUID>(below_element_uuid), sd);
}

void SEngine::handleRemoveElementsCommand(
    const proto::RemoveElementsCommand& cmd) {
  if (CheckBlockedState()) return;

  std::vector<UUID> uuids_to_remove;
  for (auto const& uuid : cmd.uuids_to_remove()) {
    uuids_to_remove.push_back(uuid);
  }
  document_->Remove(uuids_to_remove).IgnoreError();
}

proto::CameraPosition SEngine::GetCameraPosition() const {
  proto::CameraPosition position;
  Camera::WriteCameraPositionProto(&position,
                                   *root_controller_->service<Camera>());
  return position;
}

void SEngine::SetCameraPosition(const Rect& position) {
  if (position.Empty()) {
    SLOG(SLOG_ERROR, "Could not set camera position to $0", position);
    return;
  }
  root_controller_->service<CameraController>()->LookAt(position.Center(),
                                                        position.Dim());
}

void SEngine::SetCameraPosition(const proto::CameraPosition& position) {
  Status status = Camera::IsValidCameraPosition(position);
  if (!status.ok()) {
    SLOG(SLOG_ERROR, "Could not set camera position: $0.", status);
    return;
  }
  root_controller_->service<CameraController>()->LookAt(
      {position.world_center().x(), position.world_center().y()},
      {position.world_width(), position.world_height()});
}

void SEngine::PageUp() {
  root_controller_->service<CameraController>()->PageUp();
}

void SEngine::PageDown() {
  root_controller_->service<CameraController>()->PageDown();
}

void SEngine::ScrollUp() {
  root_controller_->service<CameraController>()->ScrollUp();
}

void SEngine::ScrollDown() {
  root_controller_->service<CameraController>()->ScrollDown();
}

proto::EngineState SEngine::getEngineState() const {
  proto::EngineState ans;
  *ans.mutable_camera_position() = GetCameraPosition();
  util::WriteToProto(ans.mutable_page_bounds(),
                     root_controller_->service<PageBounds>()->Bounds());
  ans.set_selection_is_live(
      root_controller_->service<ToolController>()->IsEditToolManipulating());
  util::WriteToProto(ans.mutable_mbr(), GetMinimumBoundingRect());
  return ans;
}

void SEngine::setViewport(const proto::Viewport& unsafe_viewport) {
  Status status = Camera::IsValidViewport(unsafe_viewport);
  if (!status.ok()) {
    SLOG(SLOG_ERROR, "Could not set viewport: $0", status);
    return;
  }
  root_controller_->Reshape({unsafe_viewport.width(), unsafe_viewport.height()},
                            unsafe_viewport.ppi(),
                            unsafe_viewport.screen_rotation());
}

void SEngine::assignFlag(const proto::Flag& flag, bool value) {
  root_controller_->service<settings::Flags>()->SetFlag(flag, value);
}

void SEngine::startImageExport(const proto::ImageExport& image_export) {
  if (image_export.max_dimension_px() <= 0) {
    SLOG(SLOG_ERROR, "invalid max dimension pixels");
    return;
  }
  Rect world_rect;
  if (image_export.has_world_rect()) {
    if (!ReadFromProto(image_export.world_rect(), &world_rect)) {
      SLOG(SLOG_ERROR, "Failed to read impage export bounds");
      return;
    }
  }
  GroupId render_only_group = kInvalidElementId;
  if (image_export.has_layer_index()) {
    auto layer_manager = registry()->GetShared<LayerManager>();
    auto render_only_group_or =
        layer_manager->GroupIdForLayerAtIndex(image_export.layer_index());
    if (!render_only_group_or.ok()) {
      SLOG(SLOG_ERROR, "Failed to get group id for index: $0",
           image_export.layer_index());
      return;
    }
    render_only_group = render_only_group_or.ValueOrDie();
  }
  ExportedImage img;
  root_controller_->Render(image_export.max_dimension_px(),
                           image_export.should_draw_background(), world_rect,
                           render_only_group, &img);
  host_->ImageExportComplete(img.size_px.x, img.size_px.y, img.bytes,
                             img.fingerprint);
}

void SEngine::exportImage(uint32_t maxPixelDimension, Rect worldRect,
                          ExportedImage* out) {
  constexpr bool shouldDrawBackground = true;
  root_controller_->Render(maxPixelDimension, shouldDrawBackground, worldRect,
                           kInvalidElementId, out);
}

void SEngine::addImageData(const proto::ImageInfo& image_info,
                           const ClientBitmap& client_bitmap) {
  if (!image_info.has_uri()) {
    SLOG(SLOG_ERROR, "Could not add image data, no URI specified.");
    return;
  }
  if (image_info.asset_type() == proto::ImageInfo::GRID ||
      image_info.asset_type() == proto::ImageInfo::TILED_TEXTURE) {
    if (client_bitmap.sizeInPx().width != client_bitmap.sizeInPx().height ||
        !util::IsPowerOf2(client_bitmap.sizeInPx().width)) {
      SLOG(SLOG_ERROR,
           "Could not add image data: grid textures must be squares whose "
           "width is a power of 2.");
      return;
    }
  }
  SLOG(SLOG_DATA_FLOW, "Uri: $0, Client image: $1", image_info.uri(),
       client_bitmap);
  root_controller_->service<GLResourceManager>()
      ->texture_manager->GenerateTexture(
          image_info.uri(), client_bitmap,
          TextureParams(image_info.asset_type()));
  root_controller_->service<LiveRenderer>()->Invalidate();
  root_controller_->service<FrameState>()->RequestFrame();
}

void SEngine::RejectTextureUri(const std::string& uri) {
  // Don't emit warnings for known-bad URIs that we generate.
  if (!absl::StartsWith(uri, "sketchology://background_")) {
    SLOG(SLOG_WARNING, "Host rejected texture URI: $0", uri);
  }
  root_controller_->service<GLResourceManager>()
      ->texture_manager->GenerateRejectedTexture(uri);
  root_controller_->service<LiveRenderer>()->Invalidate();
  root_controller_->service<FrameState>()->RequestFrame();
}

void SEngine::evictImageData(const std::string& uri) {
  SLOG(SLOG_DATA_FLOW, "Evicting URI $0", uri.c_str());
  TextureInfo info(uri);
  root_controller_->service<GLResourceManager>()->texture_manager->Evict(info);
}

void SEngine::evictAllTextures() {
  root_controller_->service<GLResourceManager>()->texture_manager->EvictAll();
}

UUID SEngine::addImageRect(const proto::ImageRect& add_image_rect) {
  if (CheckBlockedState()) return kInvalidUUID;

  Rect r;
  if (!ReadFromProto(add_image_rect.rect(), &r)) {
    SLOG(SLOG_ERROR, "Failed to read addImageRect bounds.");
    return kInvalidUUID;
  }
  if (r.Area() <= 0) {
    SLOG(SLOG_ERROR, "Garbage addImageRect bounds.");
    return kInvalidUUID;
  }
  ElementAttributes attributes;
  if (!ReadFromProto(add_image_rect.attributes(), &attributes)) {
    SLOG(SLOG_ERROR, "Failed to read element attributes.");
    return kInvalidUUID;
  }
  auto scene_graph = root_controller_->service<SceneGraph>();
  GroupId group_id = kInvalidElementId;
  if (add_image_rect.has_group_uuid() &&
      static_cast<UUID>(add_image_rect.group_uuid()) != kInvalidUUID) {
    group_id = scene_graph->ElementIdFromUUID(add_image_rect.group_uuid());
  }
  return root_controller_->AddImageRect(r, add_image_rect.rotation_radians(),
                                        add_image_rect.bitmap_uri(), attributes,
                                        group_id);
}

UUID SEngine::addText(const proto::text::AddText& unsafe_add_text) {
  if (CheckBlockedState()) return kInvalidUUID;

  text::TextSpec text;
  if (!ReadFromProto(unsafe_add_text.text(), &text)) {
    SLOG(SLOG_ERROR, "Failed to read text proto");
    return kInvalidUUID;
  }
  Rect world_rect;
  if (!ReadFromProto(unsafe_add_text.bounds_world(), &world_rect)) {
    SLOG(SLOG_ERROR, "Failed to read text bounds");
    return kInvalidUUID;
  }
  auto scene_graph = root_controller_->service<SceneGraph>();
  UUID uuid = scene_graph->GenerateUUID();
  GroupId group_id = kInvalidElementId;
  if (unsafe_add_text.has_group_uuid() &&
      static_cast<UUID>(unsafe_add_text.group_uuid()) != kInvalidUUID) {
    group_id = scene_graph->ElementIdFromUUID(unsafe_add_text.group_uuid());
  }

  return root_controller_->AddTextRect(text, world_rect, group_id, uuid);
}

void SEngine::beginTextEditing(const UUID& uuid) {
  root_controller_->service<TextTextureProvider>()->BeginEditing(uuid);
}

void SEngine::updateText(const proto::text::UpdateText& unsafe_update_text) {
  if (CheckBlockedState()) return;

  text::TextSpec text;
  if (!ReadFromProto(unsafe_update_text.text(), &text)) {
    SLOG(SLOG_ERROR, "Failed to read text proto");
    return;
  }

  if (unsafe_update_text.has_bounds_world()) {
    Rect world_rect;
    if (!ReadFromProto(unsafe_update_text.bounds_world(), &world_rect)) {
      SLOG(SLOG_ERROR, "Failed to read text bounds");
      return;
    }

    root_controller_->UpdateText(unsafe_update_text.uuid(), text, world_rect);
  } else if (unsafe_update_text.has_relative_size()) {
    if (unsafe_update_text.relative_size().width_multiplier() <= 0 ||
        unsafe_update_text.relative_size().height_multiplier() <= 0) {
      SLOG(SLOG_ERROR, "Invalid text height/width multiplier");
      return;
    }
    root_controller_->UpdateText(
        unsafe_update_text.uuid(), text,
        unsafe_update_text.relative_size().width_multiplier(),
        unsafe_update_text.relative_size().height_multiplier());
  } else {
    SLOG(SLOG_ERROR, "Ignoring text update with no position/size information");
  }
}

void SEngine::setToolParams(const proto::ToolParams& unsafe_proto) {
  root_controller_->SetToolParams(unsafe_proto).IgnoreError();
}

void SEngine::setOutOfBoundsColor(
    const proto::OutOfBoundsColor& out_of_bounds_color) {
  glm::vec4 color = UintToVec4RGBA(out_of_bounds_color.rgba());
  SLOG(SLOG_DATA_FLOW, "Setting out of bounds color to (r,g,b,a)=$0", color);
  color = RGBtoRGBPremultiplied(color);
  root_controller_->SetOutOfBoundsColor(color);
  root_controller_->service<LiveRenderer>()->Invalidate();
}

void SEngine::setGrid(const proto::GridInfo& grid_info) {
  if (CheckBlockedState()) return;

  if (!document_->SetGrid(grid_info)) {
    SLOG(SLOG_ERROR, "could not set grid; see logs");
  }
}

void SEngine::clearGrid() {
  if (CheckBlockedState()) return;

  setGrid(proto::GridInfo());
}

void SEngine::addSequencePoint(const proto::SequencePoint& sequence_point) {
  root_controller_->AddSequencePoint(sequence_point.id());
}

void SEngine::setCallbackFlags(const proto::SetCallbackFlags& callback_flags) {
  SourceDetails details;
  if (!ReadFromProto(callback_flags.source_details(), &details)) {
    SLOG(SLOG_ERROR, "could not read source details");
    return;
  }
  CallbackFlags flags;
  if (!ReadFromProto(callback_flags.callback_flags(), &flags)) {
    SLOG(SLOG_ERROR, "could not read callback flags");
    return;
  }
  root_controller_->service<SceneGraph>()
      ->GetElementNotifier()
      ->SetCallbackFlags(details, flags);
}

void SEngine::setOutlineExportEnabled(bool enabled) {
  auto flags = root_controller_->service<SceneGraph>()
                   ->GetElementNotifier()
                   ->GetCallbackFlags(SourceDetails::FromEngine());

  flags.attach_uncompressed_outline = enabled;

  root_controller_->service<SceneGraph>()
      ->GetElementNotifier()
      ->SetCallbackFlags(SourceDetails::FromEngine(), flags);
}

void SEngine::setHandwritingDataEnabled(bool enabled) {
  auto flags = root_controller_->service<SceneGraph>()
                   ->GetElementNotifier()
                   ->GetCallbackFlags(SourceDetails::FromEngine());

  flags.attach_compressed_input_points = enabled;

  root_controller_->service<SceneGraph>()
      ->GetElementNotifier()
      ->SetCallbackFlags(SourceDetails::FromEngine(), flags);
}

void SEngine::setCameraBoundsConfig(
    const proto::CameraBoundsConfig& camera_bounds_config) {
  if (!BoundsCheckIncInc(camera_bounds_config.fraction_padding(), 0,
                         kMaxCameraConfigPadding)) {
    SLOG(SLOG_ERROR, "Fraction padding out of allowed range.");
    return;
  }
  if (!BoundsCheckIncInc(camera_bounds_config.margin_left_px(), 0,
                         kMaxCameraConfigMarginPx)) {
    SLOG(SLOG_ERROR, "Left margin out of allowed range.");
    return;
  }
  if (!BoundsCheckIncInc(camera_bounds_config.margin_bottom_px(), 0,
                         kMaxCameraConfigMarginPx)) {
    SLOG(SLOG_ERROR, "Bottom margin out of allowed range.");
    return;
  }
  if (!BoundsCheckIncInc(camera_bounds_config.margin_right_px(), 0,
                         kMaxCameraConfigMarginPx)) {
    SLOG(SLOG_ERROR, "Right margin out of allowed range.");
    return;
  }
  if (!BoundsCheckIncInc(camera_bounds_config.margin_top_px(), 0,
                         kMaxCameraConfigMarginPx)) {
    SLOG(SLOG_ERROR, "Top margin out of allowed range.");
    return;
  }
  Margin margin;
  margin.left = camera_bounds_config.margin_left_px();
  margin.right = camera_bounds_config.margin_right_px();
  margin.bottom = camera_bounds_config.margin_bottom_px();
  margin.top = camera_bounds_config.margin_top_px();
  root_controller_->service<CameraConstraints>()->SetFractionPaddingZoomedOut(
      camera_bounds_config.fraction_padding());
  root_controller_->service<CameraConstraints>()->SetZoomBoundsMarginPx(margin);
}

void SEngine::SelectElement(const std::string& uuid) {
  root_controller_->SelectElement(uuid);
}

void SEngine::deselectAll() { root_controller_->DeselectAll(); }

void SEngine::CommitCrop() {
  if (CheckBlockedState()) return;

  root_controller_->service<tools::CropController>()->Commit();
}

void SEngine::SetCrop(const proto::Rect& crop_rect) {
  if (CheckBlockedState()) return;

  Rect new_crop;
  if (!ReadFromProto(crop_rect, &new_crop)) {
    SLOG(SLOG_ERROR, "Could not set crop Rect, as it could not be read.");
    return;
  }
  if (new_crop.Area() <= 0) {
    SLOG(SLOG_ERROR, "Could not set crop Rect, area cannot be zero.");
    return;
  }
  root_controller_->service<tools::CropController>()->SetCrop(new_crop);
}

void SEngine::handleElementAnimation(const proto::ElementAnimation& animation) {
  if (CheckBlockedState()) return;

  auto elem_anim_controller =
      registry()->GetShared<ElementAnimationController>();
  auto graph = registry()->GetShared<SceneGraph>();
  RunElementAnimation(animation, graph, elem_anim_controller);
}

void SEngine::AddTextureRequestHandler(
    const std::string& handler_id,
    std::shared_ptr<ITextureRequestHandler> handler) {
  root_controller_->service<GLResourceManager>()
      ->texture_manager->AddTextureRequestHandler(handler_id,
                                                  std::move(handler));
}

ITextureRequestHandler* SEngine::GetTextureRequestHandler(
    const std::string& handler_id) const {
  return root_controller_->service<GLResourceManager>()
      ->texture_manager->GetTextureRequestHandler(handler_id);
}

void SEngine::RemoveTextureRequestHandler(const std::string& handler_id) {
  root_controller_->service<GLResourceManager>()
      ->texture_manager->RemoveTextureRequestHandler(handler_id);
}

bool SEngine::SetLayerVisibility(int index, bool visible) {
  if (CheckBlockedState()) return false;

  auto layer_manager = registry()->GetShared<LayerManager>();
  auto scene_graph = root_controller_->service<SceneGraph>();

  auto group_id_or = layer_manager->GroupIdForLayerAtIndex(index);
  if (!group_id_or.ok()) {
    SLOG(SLOG_ERROR, "Invalid index, $0, passed to SetLayerVisibility.", index);
    return false;
  }

  proto::ElementVisibilityMutations mutations;
  AppendElementMutation(
      scene_graph->UUIDFromElementId(group_id_or.ValueOrDie()), visible,
      &mutations);
  return document_->ApplyMutations(mutations).ok();
}

bool SEngine::SetLayerOpacity(int index, int opacity) {
  if (CheckBlockedState()) return false;

  auto layer_manager = registry()->GetShared<LayerManager>();
  auto scene_graph = root_controller_->service<SceneGraph>();

  auto group_id_or = layer_manager->GroupIdForLayerAtIndex(index);
  if (!group_id_or.ok()) {
    SLOG(SLOG_ERROR, "Invalid index, $0, passed to SetLayerOpacity.", index);
    return false;
  }

  proto::ElementOpacityMutations mutations;
  AppendElementMutation(
      scene_graph->UUIDFromElementId(group_id_or.ValueOrDie()), opacity,
      &mutations);
  return document_->ApplyMutations(mutations).ok();
}

void SEngine::SetActiveLayer(int index) {
  if (CheckBlockedState()) return;

  auto layer_manager = registry()->GetShared<LayerManager>();
  auto status = layer_manager->SetActiveLayer(index);

  if (!status) {
    SLOG(SLOG_ERROR, "Failed to set active layer to $0: $1", index, status);
  }

  root_controller_->service<LiveRenderer>()->Invalidate();
}

bool SEngine::AddLayer() {
  if (CheckBlockedState()) return false;

  auto layer_manager = registry()->GetShared<LayerManager>();
  auto status_or = layer_manager->AddLayer(SourceDetails::FromEngine());
  if (!status_or.ok()) {
    SLOG(SLOG_ERROR, "Failed to create layer: $0", status_or.status());
  }
  return status_or.ok();
}

bool SEngine::MoveLayer(int from_index, int to_index) {
  if (CheckBlockedState()) return false;

  auto layer_manager = registry()->GetShared<LayerManager>();
  auto scene_graph = root_controller_->service<SceneGraph>();

  auto group_id_or = layer_manager->GroupIdForLayerAtIndex(from_index);
  if (!group_id_or.ok()) {
    SLOG(SLOG_ERROR, "Invalid from_index, $0, passed to MoveLayer.",
         from_index);
    return false;
  }

  GroupId below_group_id;
  if (to_index == layer_manager->NumLayers()) {
    below_group_id = kInvalidElementId;
  } else {
    auto below_group_id_or = layer_manager->GroupIdForLayerAtIndex(to_index);
    if (!below_group_id_or.ok()) {
      SLOG(SLOG_ERROR, "Invalid to_index, $0, passed to MoveLayer.", to_index);
      return false;
    }
    below_group_id = below_group_id_or.ValueOrDie();
  }

  proto::ElementZOrderMutations mutations;
  AppendElementMutation(
      scene_graph->UUIDFromElementId(group_id_or.ValueOrDie()),
      scene_graph->UUIDFromElementId(below_group_id), &mutations);
  return document_->ApplyMutations(mutations).ok();
}

bool SEngine::RemoveLayer(int index) {
  if (CheckBlockedState()) return false;

  auto layer_manager = registry()->GetShared<LayerManager>();
  auto status = layer_manager->RemoveLayer(index, SourceDetails::FromEngine());
  if (!status.ok()) {
    SLOG(SLOG_ERROR, "Failed to remove layer: $0", status);
  }
  return status.ok();
}

proto::LayerState SEngine::GetLayerState() {
  auto layer_manager = registry()->GetShared<LayerManager>();
  proto::LayerState layer_state;

  if (layer_manager->IsActive()) {
    auto scene_graph = root_controller_->service<SceneGraph>();

    for (int i = 0; i < layer_manager->NumLayers(); i++) {
      proto::LayerState_Layer* layer = layer_state.add_layers();
      auto group_id_or = layer_manager->GroupIdForLayerAtIndex(i);
      if (!group_id_or.ok()) {
        SLOG(SLOG_ERROR, "Invalid layer index, $0, in GetLayerState.", i);
        continue;
      }
      auto group_id = group_id_or.ValueOrDie();
      layer->set_uuid(scene_graph->UUIDFromElementId(group_id));
      layer->set_opacity(scene_graph->Opacity(group_id));
      layer->set_visibility(scene_graph->Visible(group_id));
    }

    auto active_group_or = layer_manager->GroupIdOfActiveLayer();
    if (!active_group_or.ok()) {
      SLOG(SLOG_ERROR, "Active layer not found in GetLayerState.");
    }
    layer_state.set_active_layer_uuid(scene_graph->UUIDFromElementId(
        active_group_or.ok() ? active_group_or.ValueOrDie()
                             : kInvalidElementId));
  }

  return layer_state;
}

Rect SEngine::GetMinimumBoundingRect() const {
  auto scene_graph = root_controller_->service<SceneGraph>();
  return scene_graph->Mbr();
}

Rect SEngine::GetMinimumBoundingRectForLayer(int index) const {
  auto scene_graph = root_controller_->service<SceneGraph>();
  auto layer_manager = registry()->GetShared<LayerManager>();
  auto group_id_or = layer_manager->GroupIdForLayerAtIndex(index);
  if (!group_id_or.ok()) {
    SLOG(SLOG_ERROR, "Group id not found for layer $0.", index);
    return Rect();
  }
  return scene_graph->MbrForGroup(group_id_or.ValueOrDie());
}

void SEngine::SetSelectionProvider(
    std::shared_ptr<ISelectionProvider> selection_provider) {
  root_controller_->SetSelectionProvider(selection_provider);
}

void SEngine::SetMouseWheelBehavior(const proto::MouseWheelBehavior& behavior) {
  auto pan_handler = root_controller_->service<PanHandler>();
  pan_handler->SetMousewheelPolicy(
      (behavior == proto::MouseWheelBehavior::SCROLLS)
          ? MousewheelPolicy::SCROLLS
          : MousewheelPolicy::ZOOMS);
}

bool SEngine::CheckBlockedState() const {
  if (registry()->Get<BlockerManager>()->IsBlocked()) {
    SLOG(SLOG_ERROR, "Attempt to mutate scene while blocked");
    return true;
  }
  return false;
}

}  // namespace ink
