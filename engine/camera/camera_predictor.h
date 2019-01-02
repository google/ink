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

#ifndef INK_ENGINE_CAMERA_CAMERA_PREDICTOR_H_
#define INK_ENGINE_CAMERA_CAMERA_PREDICTOR_H_

#include <memory>

#include "ink/engine/camera/camera.h"
#include "ink/engine/util/signal_filters/exp_moving_avg.h"

namespace ink {

class CameraPredictor {
 public:
  // A current camera coverage of less than this amount will trigger render
  // camera prediction.
  static const float kCoverageThreshold;

  // If prediction is triggered, the Camera will zoom by at least this much.
  static const float kMinZoomPerPrediction;

 public:
  explicit CameraPredictor(float smoothing = 0.5f);
  Camera Predict(const Camera& current_cam);
  void Update(const Camera& current_frame_cam, const Camera& last_frame_camera);

 private:
  signal_filters::ExpMovingAvg<float, float> filter_;
};

}  // namespace ink

#endif  // INK_ENGINE_CAMERA_CAMERA_PREDICTOR_H_
