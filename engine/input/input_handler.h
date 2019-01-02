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

#ifndef INK_ENGINE_INPUT_INPUT_HANDLER_H_
#define INK_ENGINE_INPUT_INPUT_HANDLER_H_

#include <cstdint>
#include <memory>

#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_dispatch.h"

namespace ink {
namespace input {

enum CaptureResult {
  CapResObserve,  // keep getting input
  CapResCapture,  // keep getting input, take capture away from others
  CapResRefuse    // refuse input until all contacts go up
};

enum class Priority {
  // Listed in order from low priority to high.
  // An input handler is able to capture input away from anything with lower
  // priority.
  Default = 0,
  Pan,
  ManipulateSelection,
  StylusEraser,
  Crop,
  TapIntercept,
  ContribImGui,
  ObserveOnly,
  UnitTestOverride  // Please leave this last - it prevents production handlers
                    // from winning priority battles with desired test handlers.
};

class IInputHandler {
 public:
  virtual ~IInputHandler() {}
  virtual CaptureResult OnInput(const InputData& data,
                                const Camera& camera) = 0;
  virtual bool RefuseAllNewInput() = 0;
  virtual Priority InputPriority() const = 0;
  virtual std::string ToString() const = 0;
};

class InputHandler : public IInputHandler {
 public:
  InputHandler();
  explicit InputHandler(Priority priority);

  // Disallow copy and assign.
  InputHandler(const InputHandler&) = delete;
  InputHandler& operator=(const InputHandler&) = delete;

  ~InputHandler() override;
  void RegisterForInput(std::shared_ptr<InputDispatch> dispatch);
  const InputDispatch* Dispatch() const { return dispatch_.get(); }

  bool RefuseAllNewInput() final { return refuse_all_new_input_; }
  void SetRefuseAllNewInput(bool refuse) { refuse_all_new_input_ = refuse; }

  Priority InputPriority() const final { return priority_; }

  inline std::string ToString() const override { return "<InputHandler>"; }

 private:
  std::shared_ptr<InputDispatch> dispatch_;
  bool is_registered_;
  uint32_t registration_token_;
  bool refuse_all_new_input_;
  const Priority priority_;
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_INPUT_HANDLER_H_
