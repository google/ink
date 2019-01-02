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

#ifndef INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_DISPATCH_H_
#define INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_DISPATCH_H_

#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/public/contrib/keyboard_input/keyboard_event.h"
#include "ink/public/contrib/keyboard_input/keyboard_handler.h"
#include "ink/public/contrib/keyboard_input/keyboard_state.h"

namespace ink {
namespace input {
namespace keyboard {

// Manages sets of keyboard listeners and keyboard event routing through them.
class Dispatch {
 public:
  virtual ~Dispatch() {}

  virtual void SendEvent(Event event) {}
  virtual const State& GetState() const { return state_; }

  virtual void BecomeFirstResponder(EventHandler* handler) {}
  virtual void DiscardFirstResponder(EventHandler* handler) {}
  virtual void StartObserving(EventHandler* handler) {}
  virtual void StopObserving(EventHandler* handler) {}

 protected:
  State state_;
};

}  // namespace keyboard
}  // namespace input
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_DISPATCH_H_
