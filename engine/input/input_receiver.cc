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

#include "ink/engine/input/input_receiver.h"

#include "ink/engine/geometry/primitives/angle_utils.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/input/sinput_helpers.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {
namespace input {

InputReceiver::InputReceiver(
    std::shared_ptr<InputDispatch> input_dispatch,
    std::shared_ptr<Camera> camera,
    std::shared_ptr<CameraController> camera_controller)
    : input_dispatch_(input_dispatch),
      camera_(camera),
      camera_controller_(camera_controller) {}

void InputReceiver::DispatchInput(InputType type, uint32_t id, uint32_t flags,
                                  double time, float screen_pos_x,
                                  float screen_pos_y) {
  // No pressure defined, provide -1.
  // No wheel delta defined, provide 0.
  // No tilt defined, provide 0.
  // No orientation defined, provide 0.
  DispatchInput(type, id, flags, time, screen_pos_x, screen_pos_y, 0, 0, -1, 0,
                0);
}

void InputReceiver::DispatchInput(InputType type, uint32_t id, uint32_t flags,
                                  double time, float screen_pos_x,
                                  float screen_pos_y, float wheel_delta_x,
                                  float wheel_delta_y, float pressure,
                                  float tilt, float orientation) {
  InputData data;
  data.time = InputTimeS(time);
  data.flags = flags;
  data.type = type;
  data.id = id;
  data.screen_pos.x = screen_pos_x;
  data.screen_pos.y = camera_->ScreenDim().y - screen_pos_y;
  data.world_pos = camera_->ConvertPosition(data.screen_pos, CoordType::kScreen,
                                            CoordType::kWorld);
  data.wheel_delta_x = wheel_delta_x;
  data.wheel_delta_y = wheel_delta_y;
  data.pressure = pressure;
  data.tilt = tilt;
  data.orientation = NormalizeAnglePositive(orientation);

  // If you change the format of this log, update
  // sketchology/tools/input/input_parser.py, it regex's over logs looking for
  // "got input:"
  SLOG(SLOG_INPUT, "got input: $0", data.ToStringExtended());

#if WEAR_HANDWRITING
  coalescer_.QueueInput(&(*input_dispatch_), *camera_, data);
#else
  input_dispatch_->Dispatch(*camera_, data);
#endif
}

void InputReceiver::DispatchInput(proto::SInputStream unsafe_input_stream) {
  bool valid = true;

  if (!ValidateCamera(unsafe_input_stream)) {
    valid = false;
    SLOG(SLOG_ERROR, "Input stream has invalid camera.");
  }
  if (unsafe_input_stream.input_size() == 0) {
    valid = false;
    SLOG(SLOG_ERROR, "Attempt to send no input!");
  }

  const int stream_ppi = unsafe_input_stream.screen_ppi();
  const glm::ivec2 stream_screen_size(unsafe_input_stream.screen_width(),
                                      unsafe_input_stream.screen_height());

  if (valid) {
    input_dispatch_->ForceAllUp(*camera_);
    Rect actual_screen(glm::vec2(0, 0), glm::vec2(camera_->ScreenDim()));
    Rect input_screen(glm::vec2(0, 0), glm::vec2(stream_screen_size));
    auto input_to_actual = input_screen.CalcTransformTo(actual_screen);
    auto transformed_ppi =
        stream_ppi * matrix_utils::GetAverageAbsScale(input_to_actual);
    auto prior_ppi = camera_->GetPPI();
    SetPPI(transformed_ppi);

    auto n_inputs = util::ProtoSizeToSizeT(unsafe_input_stream.input_size());
    for (size_t i = 0; i < n_inputs; i++) {
      SInput sinput;
      if (util::ReadFromProto(unsafe_input_stream.input(i), &sinput)) {
        glm::vec2 corrected_pos =
            geometry::Transform(sinput.screen_pos, input_to_actual);
        DispatchInput(sinput.type, sinput.id, sinput.flags,
                      static_cast<double>(sinput.time_s), corrected_pos.x,
                      corrected_pos.y, sinput.wheel_delta_x,
                      sinput.wheel_delta_y, sinput.pressure, sinput.tilt,
                      sinput.orientation);
      }
    }

    input_dispatch_->ForceAllUp(*camera_);
    SetPPI(prior_ppi);
  }
}

void InputReceiver::DispatchInput(
    const proto::PlaybackStream& unsafe_playback_stream,
    const bool force_camera) {
  if (!unsafe_playback_stream.has_initial_camera()) {
    SLOG(SLOG_ERROR, "Playback stream has no initial camera");
    return;
  }

  if (unsafe_playback_stream.events_size() == 0) {
    SLOG(SLOG_ERROR, "Attempt to send no input!");
    return;
  }

  const float prior_ppi = camera_->GetPPI();
  input_dispatch_->ForceAllUp(*camera_);

  if (force_camera) {
    Status status =
        Camera::IsValidCameraSettings(unsafe_playback_stream.initial_camera());
    if (status.ok()) {
      const auto& cam_position =
          unsafe_playback_stream.initial_camera().position();
      camera_controller_->LookAt(
          {cam_position.world_center().x(), cam_position.world_center().y()},
          {cam_position.world_width(), cam_position.world_height()});
    } else {
      SLOG(SLOG_ERROR, "Could not read initial_camera from playback stream: $0",
           status);
    }
  }
  if (!ProcessPlaybackStream(
          unsafe_playback_stream,
          /*camera_on_input_fn=*/
          [this, force_camera](const size_t, const Camera& new_camera) {
            if (force_camera) {
              camera_controller_->LookAt(new_camera.WorldWindow());
            }
            return true;
          }, /*sinput_fn=*/
          [this](const size_t, const Camera& stream_camera, SInput sinput) {
            glm::vec2 corrected_pos = sinput.screen_pos;
            // We have to flip the Y coordinate before sending this SInput
            // onward because the coordinate system that input events live in
            // has a Y axis that is flipped with respect to drawing. This is
            // corrected transparently by the DispatchInput override that we
            // call below, but when we are applying a transformation in drawing
            // space we need to flip the SInput into drawing space, transform,
            // and then flip it back.
            corrected_pos.y = stream_camera.ScreenDim().y - corrected_pos.y;
            corrected_pos = geometry::Transform(
                corrected_pos,
                camera_->WorldToScreen() * stream_camera.ScreenToWorld());
            corrected_pos.y = camera_->ScreenDim().y - corrected_pos.y;
            DispatchInput(sinput.type, sinput.id, sinput.flags,
                          static_cast<double>(sinput.time_s), corrected_pos.x,
                          corrected_pos.y, sinput.wheel_delta_x,
                          sinput.wheel_delta_y, sinput.pressure, sinput.tilt,
                          sinput.orientation);
            return true;
          })) {
    SLOG(SLOG_ERROR,
         "PlaybackStream dispatch encountered an error during playback");
  }

  input_dispatch_->ForceAllUp(*camera_);
  SetPPI(prior_ppi);
}

void InputReceiver::SetPPI(const float ppi) {
  if (ppi != camera_->GetPPI()) {
    camera_->SetPPI(ppi);
  }
}

}  // namespace input
}  // namespace ink
