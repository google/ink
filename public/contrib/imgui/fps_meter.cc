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

#include "ink/public/contrib/imgui/fps_meter.h"

#include <numeric>
#include "third_party/absl/memory/memory.h"
#include "third_party/absl/strings/str_format.h"
#include "third_party/dear_imgui/imgui.h"

namespace ink {
namespace imgui {

// What rate do we want observations to be recorded at? 60Hz
constexpr double kObservationRate = 1.0 / 60.0;
constexpr double kFilterWindowSize = 0.5;

FPSMeter::FPSMeter() { observed_fpses_.resize(300, 0); }

void FPSMeter::Update(FrameTimeS t) {
  if (!dt_filter_) {
    dt_filter_ = absl::make_unique<
        signal_filters::TimeVariantMovingAvg<double, FrameTimeS>>(

        kObservationRate, t, kFilterWindowSize);
    last_time_ = t;
    // All calcuation are based on dt. This doesn't exist the first frame, so
    // just return.
    return;
  }

  auto dt = static_cast<double>(t - last_time_);
  last_dt_ = dt;
  last_time_ = t;
  dt_filter_->Sample(dt, t);

  last_fps_idx_err_ += dt;
  while (last_fps_idx_err_ > kObservationRate) {
    observed_fpses_[last_fps_idx_ % observed_fpses_.size()] =
        1.0 / dt_filter_->Value();
    last_fps_idx_++;
    last_fps_idx_err_ -= kObservationRate;
  }

  // Set the leading graph edge to 0 s.t. the update position is visible.
  observed_fpses_[last_fps_idx_ % observed_fpses_.size()] = 0;
  last_fps_idx_ = last_fps_idx_ % observed_fpses_.size();
}

void FPSMeter::Draw(const Camera& cam, FrameTimeS t) const {
  ImGui::BeginGroup();
  std::string fps_str;
  if (last_dt_ > 0) {
    auto instant_framerate = 1.0 / last_dt_;
    auto filtered_framerate = 1.0 / dt_filter_->Value();

    fps_str = absl::StrFormat(
        "%.0f%s FPS (%0.2f ms/f)", std::min(60.0, instant_framerate),
        filtered_framerate > 60 ? "+" : "", 1000.0f / instant_framerate);
  } else {
    fps_str = absl::StrFormat("Initializing...");
  }
  float screen_width =
      cam.ConvertDistance(400, DistanceType::kDp, DistanceType::kScreen);
  float screen_height =
      cam.ConvertDistance(100, DistanceType::kDp, DistanceType::kScreen);
  ImGui::PlotLines("", observed_fpses_.data(), observed_fpses_.size(), 0,
                   fps_str.c_str(), 0.0f, 60.0f,
                   ImVec2(screen_width, screen_height));
  ImGui::EndGroup();
}

}  // namespace imgui
}  // namespace ink
