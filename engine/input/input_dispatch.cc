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

#include "ink/engine/input/input_dispatch.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {
namespace input {

InputDispatch::InputDispatch(std::shared_ptr<settings::Flags> flags)
    : flags_(std::move(flags)), next_token_(1), pen_used_(false) {}

uint32_t InputDispatch::RegisterHandler(IInputHandler* handler) {
  auto t = next_token_++;
  token_to_handler_[t] = handler;

  OnHandlersChanged();
  return t;
}

void InputDispatch::OnHandlersChanged() {
  sorted_handlers_.clear();
  for (auto p : token_to_handler_) sorted_handlers_.push_back(p.second);

  // Sort in descending order -- highest priority should go first.
  std::sort(sorted_handlers_.begin(), sorted_handlers_.end(),
            [](IInputHandler* a, IInputHandler* b) {
              return static_cast<int>(a->InputPriority()) >
                     static_cast<int>(b->InputPriority());
            });
}

void InputDispatch::UnregisterHandler(uint32_t token) {
  if (token == 0) {
    return;
  }
  if (token_to_handler_.find(token) == token_to_handler_.end()) {
    SLOG(SLOG_WARNING,
         "trying to unregisterHandler handler $0, but it was not found", token);
    return;
  }

  // If this handler has captured any input, remove the captures
  auto handler = token_to_handler_[token];
  ReleaseCapturedStreams(handler);

  // remove all the other possible refs to the handler
  token_to_handler_.erase(token);
  refused_handlers_.erase(handler);

  OnHandlersChanged();
}

void InputDispatch::ForceAllUp(const Camera& cam) {
  while (!input_id_to_last_input_.empty()) {
    auto data = input_id_to_last_input_.begin()->second;
    ASSERT(data.Get(Flag::InContact));
    Dispatch(cam, InputData::MakeCancel(data));
  }
}

void InputDispatch::Dispatch(const Camera& cam, InputData data) {
  SLOG(SLOG_INPUT, "dispatching $0", data.ToStringExtended());

  // Update/Validate the packet state

  auto li = input_id_to_last_input_.find(data.id);
  InputData* last = nullptr;
  if (li != input_id_to_last_input_.end()) {
    last = &li->second;
  }
  InputData::SetLastPacketInfo(&data, last);
  if (!InputData::CorrectPacket(&data, last)) {
    return;
  }

  if (flags_->GetFlag(settings::Flag::AutoPenModeEnabled) && !pen_used_ &&
      data.type == input::InputType::Pen) {
    flags_->SetFlag(settings::Flag::PenModeEnabled, true);
    pen_used_ = true;
  }

  if (data.Get(Flag::TDown) && input_id_to_last_input_.empty())
    data.Set(Flag::Primary, true);

  if (data.Get(Flag::InContact)) {
    input_id_to_last_input_[data.id] = data;
  } else {
    input_id_to_last_input_.erase(data.id);
  }
  data.n_down = input_id_to_last_input_.size();

  IInputHandler* capturer = nullptr;
  if (input_id_to_capturer_.count(data.id) > 0) {
    capturer = input_id_to_capturer_[data.id];
  }

  // At this point we've finished correcting the input / setting up our state
  // Figure out whom to send the packet to and send it out
  for (auto& h : sorted_handlers_) {
    if (refused_handlers_.find(h) != refused_handlers_.end()) {
      // The handler has already refused all inputs.
      continue;
    } else if (h->RefuseAllNewInput()) {
      // A handler can refuse capture until all inputs go up.
      AddRefusedHandler(h);
      if (h == capturer) capturer = nullptr;
      continue;
    } else if (capturer && h != capturer &&
               static_cast<int>(h->InputPriority()) <=
                   static_cast<int>(capturer->InputPriority())) {
      // If the handler's priority is less than or equal to the capturer's
      // priority, don't send the input data packet through to it.
      continue;
    }

    auto capture = h->OnInput(data, cam);
    if (capture == CapResCapture) {
      // Make sure that the observe all priorities can't capture.
      ASSERT(h->InputPriority() != Priority::ObserveOnly);

      if (h != capturer) {
        // an observer has requested late capture --
        // send an up|cancel to the previous owner
        if (capturer) {
          InputData cancel = InputData::MakeCancel(data);
          SLOG(SLOG_INPUT,
               "changing input capturer for ($0), sending cancel ($1) to prior "
               "owner",
               data, cancel);
          CaptureResult result = capturer->OnInput(cancel, cam);
          ASSERT(result != CapResCapture);
          if (result == CapResRefuse) AddRefusedHandler(capturer);
        }
        capturer = h;
        input_id_to_capturer_[data.id] = capturer;
      }
    } else if (capture == CapResRefuse) {
      // handler doesn't want any more input from this stream
      AddRefusedHandler(h);
      if (h == capturer) capturer = nullptr;
    }
  }

  // We're done sending the packet now, update out state
  if (!data.Get(Flag::InContact)) {
    input_id_to_capturer_.erase(data.id);
    if (data.n_down == 0) refused_handlers_.clear();
  }
}

void InputDispatch::ReleaseCapturedStreams(IInputHandler* handler) {
  for (auto it = input_id_to_capturer_.begin();
       it != input_id_to_capturer_.end();) {
    if (it->second == handler) {
      it = input_id_to_capturer_.erase(it);
    } else {
      ++it;
    }
  }
}

void InputDispatch::AddRefusedHandler(IInputHandler* handler) {
  refused_handlers_.insert(handler);
  ReleaseCapturedStreams(handler);
}

}  // namespace input
}  // namespace ink
