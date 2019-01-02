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

#ifndef INK_PUBLIC_CONTRIB_IMGUI_FPS_METER_H_
#define INK_PUBLIC_CONTRIB_IMGUI_FPS_METER_H_

#include <vector>
#include "ink/engine/camera/camera.h"
#include "ink/engine/util/signal_filters/time_variant_moving_avg.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace imgui {

// Graphs FPS over time.
class FPSMeter {
 public:
  FPSMeter();

  // Updates the graph data.
  void Update(FrameTimeS t);

  // Graphs FPS in an ImGui::Group.
  void Draw(const Camera& cam, FrameTimeS t) const;

 private:
  // A history of fps samples. Each slot in the vector represents a sample
  // over 1/60th of a second. This is used as a circular buffer.
  std::vector<float> observed_fpses_;

  // Where are we writing to in observed_fpses?
  size_t last_fps_idx_ = 0;

  // observed_fpses_ represents samples at a particular rate -- but we can only
  // write to the buffer at a divergent rate. Keep track of accumulated error to
  // avoid aliasing artifacts.
  // Think of this as how much we want to write between indexes in
  // observed_fpses_.
  double last_fps_idx_err_ = 0;

  // Low pass filter over observed delta frame times.
  std::unique_ptr<signal_filters::TimeVariantMovingAvg<double, FrameTimeS>>
      dt_filter_;

  FrameTimeS last_time_;
  double last_dt_ = 0;
};

}  // namespace imgui
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_IMGUI_FPS_METER_H_
