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

#include "ink/engine/realtime/magic_eraser_stylus_handler.h"

namespace ink {

MagicEraserStylusHandler::MagicEraserStylusHandler(
    std::shared_ptr<settings::Flags> flags,
    std::shared_ptr<input::InputDispatch> dispatch,
    std::shared_ptr<SceneGraph> scene_graph,
    std::shared_ptr<IDbgHelper> dbg_helper,
    std::shared_ptr<LayerManager> layer_manager)
    : magic_eraser_(dispatch, scene_graph, dbg_helper, layer_manager, true) {
  flags->AddListener(this);
}

void MagicEraserStylusHandler::OnFlagChanged(settings::Flag which,
                                             bool new_value) {
  if (which == settings::Flag::ReadOnlyMode) {
    magic_eraser_.Enable(!new_value);
  }
}

}  // namespace ink
