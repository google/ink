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

#ifndef RESEARCH_INK_LF_GUI_IMGUI_FRAMEWISE_INPUT_H_
#define RESEARCH_INK_LF_GUI_IMGUI_FRAMEWISE_INPUT_H_

#include <string>
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/scene/frame_state/frame_state.h"

namespace ink {
namespace imgui {

// Keeps track of all input that happens over the course of a frame.
// The semantics of this class are a bit strange -- basically we're trying
// to map high freq touch/pen/mouse input to the imgui framework, which expects
// mouse-only updates exactly once a frame.
//
// The basic idea is we're trying to track 3 things:
//   - Mouse wheel
//   - Primary (if it exists)
//   - Hover (from mouse or pen)
//
// WARNING! This class is relatively simple and does not handle input edge cases
// well! Examples of current known failure modes:
//   - No capture on wheel events.
//   - Weird behavior if a down and up come in the same frame.
// Use this class at your own risk. It's intended as a debug tool only.
class FramewiseInput : public input::InputHandler {
 public:
  enum class TrackingState { kTrackingPrimary, kHovering, kNone } state_;

  explicit FramewiseInput(std::shared_ptr<input::InputDispatch> dispatch,
                          std::shared_ptr<FrameState> frame_state);

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;

  // Starts a new frame of input capture. Note this is potentially divergent
  // from any actual drawing frames, and totally dependent on the user of the
  // object.
  void NewFrame();

  // Should this input handler capture primary input?
  void SetCapture(bool capture);

  bool SawAnyNonWheelInputLastFrame() const;

  // Returns the last (x) packet seen. This is for the *last* frame considered,
  // not the current frame. As such the result will stay stable over the course
  // of the frame.
  input::InputData LastInput() const;
  TrackingState LastTrackingState() const;

  // Returns the cumulative wheel delta seen over the course of the last frame.
  float LastWheelDelta() const;

  // Sets whether this input handler actually does anything. By
  // default it is enabled. When disabled, its OnInput method is
  // short-circuited in order to keep it from consuming any input
  // events.
  void SetEnabled(bool b);

  inline std::string ToString() const override { return "<FramewiseInput>"; }

 private:
  std::shared_ptr<FrameState> frame_state_;
  TrackingState current_state_ = TrackingState::kNone;
  TrackingState last_state_ = TrackingState::kNone;
  input::InputData current_input_;
  input::InputData last_input_;
  float current_wheel_delta_ = 0;
  float last_wheel_delta_ = 0;
  bool saw_any_input_non_wheel_input_this_frame_ = false;
  bool saw_any_input_non_wheel_input_last_frame_ = false;
  bool saw_down_this_frame_ = false;
  bool capture_ = false;
  bool enabled_ = true;
};

}  // namespace imgui
}  // namespace ink

#endif  // RESEARCH_INK_LF_GUI_IMGUI_FRAMEWISE_INPUT_H_
