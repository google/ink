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

#include "ink/engine/camera/camera_predictor.h"

#include <algorithm>

#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/primitives/rect.h"

namespace ink {

const float CameraPredictor::kCoverageThreshold = 0.93f;
// A Scale factor < 1 is a zoom out, i.e., render more of the scene.
const float CameraPredictor::kMinZoomPerPrediction = 0.3f;

CameraPredictor::CameraPredictor(float smoothing) : filter_(1.0f, smoothing) {}

Camera CameraPredictor::Predict(const Camera& current_cam) {
  auto cvg = filter_.Value();

  Camera res(current_cam);

  // if we have incomplete coverage, try zooming out
  if (cvg < kCoverageThreshold) {
    auto zm = std::max(kMinZoomPerPrediction, cvg / 2.0f);
    res.Scale(1.0f / zm, current_cam.WorldCenter());
  }

  return res;
}

void CameraPredictor::Update(const Camera& current_frame_cam,
                             const Camera& last_frame_camera) {
  auto old_r = geometry::Envelope(last_frame_camera.WorldRotRect());
  auto new_r = geometry::Envelope(current_frame_cam.WorldRotRect());

  float coverage_ratio = 0;
  Rect intersection;
  if (geometry::Intersection(old_r, new_r, &intersection))
    coverage_ratio = intersection.Area() / new_r.Area();

  filter_.Sample(coverage_ratio);
}

}  // namespace ink
