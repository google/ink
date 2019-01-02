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

#include "ink/engine/input/input_model_params.h"

#include "third_party/absl/strings/str_format.h"

namespace ink {
namespace input {

namespace {

// Note: On platforms other than Android, we only get the input at 60hz, so we
// end up sampling at that rate but include a cap slightly higher to avoid
// incorrectly dropping points that come in after 15ms. On Android we send
// the historical coalesced events so we can attempt to sample down to 60hz.
// See b/29048323.
#ifdef __ANDROID__
constexpr int kMaxSampleHzTouch = 60;
#else
constexpr int kMaxSampleHzTouch = 80;
#endif
constexpr int kMaxSampleHzMouse = 90;
constexpr int kMaxSampleHzPen = 60;

constexpr DurationS kPredictDurationMouse = 1.0 / kMaxSampleHzMouse;
constexpr DurationS kPredictDurationTouch = 2.0 / kMaxSampleHzTouch;
constexpr DurationS kPredictDurationPen = 0.025;

}  // namespace

InputModelParams::InputModelParams(InputType type)
    : num_interpolation_pts_(3),
      speed_mod_for_stroke_end_(0.75f),
      max_points_after_up_(21),
      drag_(0.4f),
      mass_(11),
      wobble_timeout_ratio_(2.4),
      wobble_slow_speed_cm_(1.31),
      wobble_fast_speed_cm_(1.44) {
  switch (type) {
    case input::InputType::Mouse:
      max_sample_hz_ = kMaxSampleHzMouse;
      predict_interval_ = kPredictDurationMouse;
      break;
    case input::InputType::Pen:
      max_sample_hz_ = kMaxSampleHzPen;
      predict_interval_ = kPredictDurationPen;
      break;
    case input::InputType::Touch:
    case input::InputType::Invalid:
      max_sample_hz_ = kMaxSampleHzTouch;
      predict_interval_ = kPredictDurationTouch;
      break;
  }
}

std::string InputModelParams::ToString() const {
  return absl::StrFormat(
      "num_interpolation_pts=%d, max_sample_hz=%.2f, "
      "speed_mod_for_stroke_end: %.2f, max_points_after_up: %d",
      num_interpolation_pts_, max_sample_hz_, speed_mod_for_stroke_end_,
      max_points_after_up_);
}

}  // namespace input
}  // namespace ink
