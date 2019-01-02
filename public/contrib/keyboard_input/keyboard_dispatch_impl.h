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

#ifndef INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_DISPATCH_IMPL_H_
#define INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_DISPATCH_IMPL_H_

#include "ink/public/contrib/keyboard_input/keyboard_dispatch.h"

namespace ink {
namespace input {
namespace keyboard {

class DispatchImpl : public Dispatch {
 public:
  using SharedDeps = service::Dependencies<FrameState>;

  explicit DispatchImpl(std::shared_ptr<FrameState> frame_state);

  // Dispatches an event through the system. The state of the keyboard at event
  // time is inferred to be last_state_ + event.
  void SendEvent(Event event);
  const State& GetState() const;

  // By default keyboard event handlers get no events.
  // They can start receiving events in the following way:
  //   - StartObserving:
  //       Observers receive all keyboard events in non-deterministic order.
  //       Observers see events after the first responder.
  //   - BecomeFirstResponder:
  //       There is always exactly 0 or 1 first responders. Becoming the first
  //       responder takes away the state from the prior handler (if there is
  //       one). The first responder sees keyboard input before any other part
  //       of the system.
  // Handlers are allowed to both be a first responder and an observer
  // simultaneously.
  //
  // Memory semantics:
  // No lifetime assumptions are made on KeyboardHandlers. They may come and go
  // as they please. The dispatch does not keep handlers alive, nor assume that
  // they remain alive.
  void BecomeFirstResponder(EventHandler* handler) override;
  void DiscardFirstResponder(EventHandler* handler) override;
  void StartObserving(EventHandler* handler) override;
  void StopObserving(EventHandler* handler) override;

 private:
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<EventDispatch<EventHandler>> first_responder_ =
      std::make_shared<EventDispatch<EventHandler>>();
  std::shared_ptr<EventDispatch<EventHandler>> observers_ =
      std::make_shared<EventDispatch<EventHandler>>();
};

}  // namespace keyboard
}  // namespace input
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_DISPATCH_IMPL_H_
