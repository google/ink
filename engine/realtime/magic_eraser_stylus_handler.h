/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_REALTIME_MAGIC_ERASER_STYLUS_HANDLER_H_
#define INK_ENGINE_REALTIME_MAGIC_ERASER_STYLUS_HANDLER_H_

#include <memory>

#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/realtime/magic_eraser.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"
#include "ink/engine/util/dbg_helper.h"

namespace ink {

// Even though this class is called a "handler", it is not actually an input
// handler. However, it does instantiate and register an input handler
// (MagicEraser).
class MagicEraserStylusHandler : public settings::FlagListener {
 public:
  using SharedDeps =
      service::Dependencies<settings::Flags, input::InputDispatch, SceneGraph,
                            IDbgHelper, LayerManager>;
  MagicEraserStylusHandler(std::shared_ptr<settings::Flags> flags,
                           std::shared_ptr<input::InputDispatch> dispatch,
                           std::shared_ptr<SceneGraph> scene_graph,
                           std::shared_ptr<IDbgHelper> dbg_helper,
                           std::shared_ptr<LayerManager> layer_manager);

  void OnFlagChanged(settings::Flag which, bool new_value) override;

 private:
  MagicEraser magic_eraser_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_MAGIC_ERASER_STYLUS_HANDLER_H_
