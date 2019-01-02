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

#ifndef INK_ENGINE_INPUT_INPUT_RECEIVER_H_
#define INK_ENGINE_INPUT_INPUT_RECEIVER_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/camera_controller/camera_controller.h"
#include "ink/engine/input/input_coalescer.h"
#include "ink/engine/input/input_dispatch.h"
#include "ink/engine/public/types/input.h"
#include "ink/engine/service/definition_list.h"
#include "ink/engine/service/registry.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {
namespace input {

// Service to receive input events, which may be in a variety of formats; do any
// format-specific processing; and send properly formatted events to lower-level
// input dispatch.
class InputReceiver {
 public:
  using SharedDeps =
      service::Dependencies<InputDispatch, Camera, CameraController>;

  InputReceiver(std::shared_ptr<InputDispatch> input_dispatch,
                std::shared_ptr<Camera> camera,
                std::shared_ptr<CameraController> camera_controller);
  InputReceiver(const InputReceiver&) = default;
  InputReceiver(InputReceiver&&) = default;
  InputReceiver& operator=(const InputReceiver&) = default;
  InputReceiver& operator=(InputReceiver&&) = default;

  void DispatchInput(InputType type, uint32_t id, uint32_t flags, double time,
                     float screen_pos_x, float screen_pos_y);

  // Orientation indicates the direction in which the stylus is pointing in
  // relation to the positive x axis. A value of 0 means the ray from the stylus
  // tip to the end is along positive x and values increase counter-clockwise.
  void DispatchInput(InputType type, uint32_t id, uint32_t flags, double time,
                     float screen_pos_x, float screen_pos_y,
                     float wheel_delta_x, float wheel_delta_y, float pressure,
                     float tilt, float orientation);

  // In general, prefer the non-proto API for input. Input is frequent enough
  // that proto allocs/encoding/decoding is noticeable.
  //
  // Do not mix usage of this API and the non-proto input API.
  // Sending input to this API will cancel any in-progress input!
  void DispatchInput(proto::SInputStream unsafe_input_stream);

  // Plays back a comprehensive stream of inputs, which may include
  // camera changes. If force_camera is set to true, then a best effort
  // is made to point the engine camera at the same world window that
  // was visible when unsafe_playback_stream was recorded.
  //
  // This method is intended for testing/debugging only. In general,
  // prefer the non-proto API for input. Input is frequent enough that
  // proto allocs/encoding/decoding is noticeable.
  //
  // Do not mix usage of this API and the non-proto input API.
  // Sending input to this API will cancel any in-progress input!
  void DispatchInput(const proto::PlaybackStream& unsafe_playback_stream,
                     bool force_camera = false);

#if WEAR_HANDWRITING
  input::InputCoalescer* GetCoalescer() { return &coalescer_; }
#endif

 private:
  void SetPPI(float ppi);

  std::shared_ptr<InputDispatch> input_dispatch_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<CameraController> camera_controller_;
#if WEAR_HANDWRITING
  input::InputCoalescer coalescer_;
#endif
};

}  // namespace input
}  // namespace ink

#endif  // INK_ENGINE_INPUT_INPUT_RECEIVER_H_
