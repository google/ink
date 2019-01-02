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

#ifndef INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_HANDLER_H_
#define INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_HANDLER_H_

#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/public/contrib/keyboard_input/keyboard_event.h"

namespace ink {
namespace input {
namespace keyboard {

// Superclass for all keyboard event handlers. See keyboard_dispatch.h
// for a description of how these events are triggered and the difference
// between observers and the first responder.
class EventHandler : public EventListener<EventHandler> {
 public:
  EventHandler() {}

  // Events dispatched to observers will be routed here.
  // Note this sees only events that are generated _after_ becoming an observer.
  // If you become an observer in response to an observed keyboard event, that
  // event will not be resent.
  virtual void HandleEventAsObserver(const Event& event) {}
  // Events dispatched to the first responder will be routed here.
  // Note this sees only events that are generated _after_ becoming the first
  // responder. If you become a first responder in response to an observed
  // keyboard event, that event will not be resent.
  virtual void HandleEventAsFirstResponder(const Event& event) {}

  // Sent when the dispatch starts considering this handler the first responder.
  virtual void BecameFirstResponder() {}
  // Sent when the dispatch stops considering this handler the first responder.
  virtual void LostFirstResponder() {}
};

}  // namespace keyboard
}  // namespace input
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_KEYBOARD_INPUT_KEYBOARD_HANDLER_H_
