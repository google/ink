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

#include "ink/public/contrib/imgui/framewise_input.h"

#include "third_party/dear_imgui/imgui.h"

namespace ink {
namespace imgui {

namespace {
bool IsHover(const input::InputData& data) { return data.n_down == 0; }
}  // namespace

FramewiseInput::FramewiseInput(std::shared_ptr<input::InputDispatch> dispatch,
                               std::shared_ptr<FrameState> frame_state)
    : input::InputHandler(input::Priority::ContribImGui),
      frame_state_(frame_state) {
  RegisterForInput(dispatch);
}

input::CaptureResult FramewiseInput::OnInput(const input::InputData& data,
                                             const Camera& camera) {
  if (!enabled_) {
    return input::CapResObserve;
  }
  if (data.Get(input::Flag::Wheel)) {
    current_wheel_delta_ += data.wheel_delta_y;
    // We really want to conditionally capture this, but it's hard to phrase as
    // there's no continuity/cancel concept for wheel events
    // in ink. The root of the problem is ImGui is only able to give a capture
    // value once per frame but ink wants a response immediately.
    // Send to everyone for now... which means double scrolling :(.
    return input::CapResObserve;
  }

  saw_any_input_non_wheel_input_this_frame_ = true;
  if (data.Get(input::Flag::TDown)) {
    saw_down_this_frame_ = true;
  }

  if (data.Get(input::Flag::Primary)) {
    current_state_ = TrackingState::kTrackingPrimary;
    current_input_ = data;
    if (saw_down_this_frame_) {
      // ImGui lags 1 frame behind actual input and only updates on the down
      // event -- give it a chance to update the desired capture behavior before
      // stealing input away from everyone else.
      return input::CapResObserve;
    }
    return capture_ ? input::CapResCapture : input::CapResObserve;
  }

  if (current_state_ != TrackingState::kTrackingPrimary) {
    current_state_ =
        IsHover(data) ? TrackingState::kNone : TrackingState::kHovering;
  }

  if (current_state_ == TrackingState::kHovering) {
    current_input_ = data;
  }

  // Poke imgui input handling, which only runs on draw frames.
  frame_state_->RequestFrame();

  return input::CapResObserve;
}

void FramewiseInput::NewFrame() {
  capture_ = false;
  last_wheel_delta_ = current_wheel_delta_;
  current_wheel_delta_ = 0;
  saw_any_input_non_wheel_input_last_frame_ =
      saw_any_input_non_wheel_input_this_frame_;
  saw_any_input_non_wheel_input_this_frame_ = false;
  saw_down_this_frame_ = false;
  if (!saw_any_input_non_wheel_input_last_frame_) {
    return;
  }
  last_state_ = current_state_;
  last_input_ = current_input_;

  if (last_state_ == TrackingState::kTrackingPrimary &&
      last_input_.Get(input::Flag::TUp)) {
    current_state_ =
        IsHover(last_input_) ? TrackingState::kNone : TrackingState::kHovering;
  }
}

void FramewiseInput::SetCapture(bool capture) { capture_ = capture; }

FramewiseInput::TrackingState FramewiseInput::LastTrackingState() const {
  return last_state_;
}

input::InputData FramewiseInput::LastInput() const { return last_input_; }

float FramewiseInput::LastWheelDelta() const { return last_wheel_delta_; }

void FramewiseInput::SetEnabled(const bool b) { enabled_ = b; }

bool FramewiseInput::SawAnyNonWheelInputLastFrame() const {
  return saw_any_input_non_wheel_input_last_frame_;
}

}  // namespace imgui
}  // namespace ink
