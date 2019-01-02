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

#include "ink/public/contrib/keyboard_input/keyboard_dispatch_impl.h"

#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace input {
namespace keyboard {

DispatchImpl::DispatchImpl(std::shared_ptr<FrameState> frame_state)
    : frame_state_(frame_state) {}

void DispatchImpl::SendEvent(Event event) {
  if (auto key_event = absl::get_if<KeyEvent>(&event)) {
    auto& vkey = key_event->virtualcode;
    auto& flags = key_event->state_flags;
    if (flags == 0) {
      SLOG(SLOG_WARNING, "ignoring no-op keyboard event. code: ", vkey);
      return;
    }
    if (flags & kTUp) {
      DCHECK_EQ(0, flags & ~kTUp);
      if (state_.IsUp(vkey)) {
        SLOG(SLOG_WARNING,
             "ignoring duplicate up keyboard event. code: ", vkey);
        return;
      }
      state_.SetDown(vkey, false);
    }
    if (flags & kTDown) {
      DCHECK_EQ(0, flags & ~kTDown);
      if (state_.IsDown(vkey)) {
        flags = kRepeat;
      } else {
        state_.SetDown(vkey, true);
      }
    }
  } else {
    auto input_event = absl::get_if<UTF8InputEvent>(&event);
    if (!input_event) {
      SLOG(SLOG_WARNING, "Got an event with no data. Ignoring.");
      return;
    }
  }

  // We've decided to dispatch the event. Make sure to request at least one more
  // frame to show the result of the key transition.
  frame_state_->RequestFrame();

  first_responder_->Send(&EventHandler::HandleEventAsFirstResponder, event);
  observers_->Send(&EventHandler::HandleEventAsObserver, event);
}

const State& DispatchImpl::GetState() const { return state_; }

void DispatchImpl::BecomeFirstResponder(EventHandler* handler) {
  if (handler->IsRegistered(first_responder_)) {
    // Do nothing -- already the first responder.
    DCHECK_EQ(1, first_responder_->size());
    return;
  }
  // Remove the last first responder.
  first_responder_->Send(&EventHandler::LostFirstResponder);
  first_responder_->Send(&EventHandler::Unregister, first_responder_);
  DCHECK_EQ(0, first_responder_->size());
  // Add the new first responder.
  handler->RegisterOnDispatch(first_responder_);
  first_responder_->Send(&EventHandler::BecameFirstResponder);
}

void DispatchImpl::DiscardFirstResponder(EventHandler* handler) {
  if (!handler->IsRegistered(first_responder_)) {
    // Do nothing -- this handler is not the first responder.
    return;
  }

  first_responder_->Send(&EventHandler::LostFirstResponder);
  first_responder_->Send(&EventHandler::Unregister, first_responder_);
  DCHECK_EQ(0, first_responder_->size());
}

void DispatchImpl::StartObserving(EventHandler* handler) {
  handler->RegisterOnDispatch(observers_);
}

void DispatchImpl::StopObserving(EventHandler* handler) {
  handler->Unregister(observers_);
}

}  // namespace keyboard
}  // namespace input
}  // namespace ink
