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

#include "ink/engine/scene/default_services.h"

#include "third_party/absl/memory/memory.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/camera_controller/camera_controller.h"
#include "ink/engine/debug_view/debug_view.h"
#include "ink/engine/geometry/spatial/sticker_spatial_index_factory.h"
#include "ink/engine/input/cursor_manager.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/input/input_modeler.h"
#include "ink/engine/input/input_receiver.h"
#include "ink/engine/input/physics_input_modeler.h"
#include "ink/engine/input/prediction/input_predictor.h"
#include "ink/engine/input/prediction/repeat_predictor.h"
#include "ink/engine/processing/blocker_manager.h"
#include "ink/engine/processing/runner/service_definition.h"
#include "ink/engine/public/host/ielement_listener.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/engine/public/host/iplatform.h"
#include "ink/engine/public/host/public_events.h"
#include "ink/engine/realtime/crop_controller.h"
#include "ink/engine/realtime/crop_mode.h"
#include "ink/engine/realtime/line_tool_data_sink.h"
#include "ink/engine/realtime/magic_eraser_stylus_handler.h"
#include "ink/engine/realtime/modifiers/line_modifier_factory.h"
#include "ink/engine/realtime/pan_handler.h"
#include "ink/engine/realtime/tool_controller.h"
#include "ink/engine/rendering/compositing/live_renderer.h"
#include "ink/engine/rendering/compositing/scene_graph_renderer.h"
#include "ink/engine/rendering/export/image_exporter.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/gl_managers/ion_graphics_manager_provider.h"
#include "ink/engine/rendering/gl_managers/text_texture_provider.h"
#include "ink/engine/rendering/strategy/rendering_strategy.h"
#include "ink/engine/scene/data/common/poly_store.h"
#include "ink/engine/scene/element_animation/element_animation_controller.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/grid_manager.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/page/page_border.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/scene/page/page_properties_notifier.h"
#include "ink/engine/scene/particle_manager.h"
#include "ink/engine/scene/root_renderer.h"
#include "ink/engine/scene/update_loop.h"
#include "ink/engine/service/definition_list.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/dbg_helper.h"
#include "ink/engine/util/dbg_input_visualizer.h"
#include "ink/engine/util/time/wall_clock.h"
#include "ink/public/contrib/keyboard_input/keyboard_dispatch.h"

namespace ink {

std::unique_ptr<service::DefinitionList> DefaultServiceDefinitions() {
  auto definitions = absl::make_unique<service::DefinitionList>();

  // Create a PublicEvents, and expose it to any service that depends on the
  // listener types it implements.
  auto public_events = std::make_shared<PublicEvents>();
  definitions->DefineExistingService<PublicEvents>(public_events);
  definitions->DefineExistingService<IElementListener>(public_events);
  definitions->DefineExistingService<IEngineListener>(public_events);
  definitions->DefineExistingService<IPagePropertiesListener>(public_events);

  definitions->DefineService<PagePropertiesNotifier>();
  definitions->DefineService<IonGraphicsManagerProvider>();
  definitions->DefineService<GLResourceManager>();
  definitions->DefineService<PageBounds>();
  definitions->DefineService<settings::Flags>();
  definitions->DefineService<input::InputDispatch>();
  definitions->DefineService<input::CursorManager>();
  definitions->DefineService<Camera>();
  definitions->DefineService<CameraController>();
  definitions->DefineService<CameraConstraints>();
  definitions->DefineService<FrameState>();
  definitions->DefineService<AnimationController>();
  definitions->DefineService<SceneGraph>();
  definitions->DefineService<LineToolDataSink>();
  definitions->DefineService<PanHandler, DefaultPanHandler>();
  definitions->DefineService<ToolController>();
  definitions->DefineService<MagicEraserStylusHandler>();
  definitions->DefineService<input::InputPredictor, input::RepeatPredictor>();
  definitions->DefineService<input::InputModeler, input::PhysicsInputModeler>();
  definitions->DefineService<LineModifierFactory>();
  definitions->DefineService<RootRenderer, RootRendererImpl>();
  definitions->DefineService<WallClockInterface, WallClock>();
  definitions->DefineService<spatial::StickerSpatialIndexFactoryInterface,
                             spatial::StickerSpatialIndexFactory>();

  definitions->DefineService<ElementAnimationController>();
  definitions->DefineService<UpdateLoop, DefaultUpdateLoop>();
  definitions->DefineService<GridManager>();
  definitions->DefineService<ParticleManager>();
  definitions->DefineService<PageBorder>();
  definitions->DefineService<tools::CropController>();
  definitions->DefineService<CropMode>();
  definitions->DefineService<PageManager>();
  definitions->DefineService<TextTextureProvider>();
  definitions->DefineService<LayerManager>();
  definitions->DefineService<input::keyboard::Dispatch>();
  definitions->DefineService<input::InputReceiver>();
  definitions->DefineService<PolyStore>();
  definitions->DefineService<ImageExporter, DefaultImageExporter>();
  definitions->DefineService<BlockerManager>();
  definitions->DefineService<LiveRenderer>();

  definitions->DefineService<IDbgHelper, NoopDbgHelper>();
#if INK_DEBUG
  definitions->DefineService<DbgInputVisualizer>();
#endif

  DefineDebugView(definitions.get());
  DefineTaskRunner(definitions.get());

  return definitions;
}

}  // namespace ink
