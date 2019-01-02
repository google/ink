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

#ifndef INK_ENGINE_UTIL_DBG_INPUT_VISUALIZER_H_
#define INK_ENGINE_UTIL_DBG_INPUT_VISUALIZER_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/dbg_helper.h"

namespace ink {

class DbgInputVisualizer : public input::InputHandler {
 public:
  using SharedDeps = service::Dependencies<input::InputDispatch, IDbgHelper>;

  DbgInputVisualizer(std::shared_ptr<input::InputDispatch> dispatch,
                     std::shared_ptr<IDbgHelper> dbg_helper);

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  void SetColor(glm::vec4 color) { color_ = color; }

  inline std::string ToString() const override {
    return "<DbgInputVisualizer>";
  }

 private:
  std::shared_ptr<IDbgHelper> dbg_helper_;
  glm::vec4 color_ = glm::vec4(0, 1, 0, .75f);
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_DBG_INPUT_VISUALIZER_H_
