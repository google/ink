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

#include "ink/engine/input/input_recorder.h"

#include "ink/engine/input/sinput.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {
namespace input {

InputRecorder::InputRecorder(const service::UncheckedRegistry& registry)
    : InputRecorder(registry.GetShared<InputDispatch>()) {}

InputRecorder::InputRecorder(std::shared_ptr<InputDispatch> dispatch)
    : InputHandler(Priority::ObserveOnly) {
  RegisterForInput(dispatch);
}

CaptureResult InputRecorder::OnInput(const InputData& data, const Camera& cam) {
  if (!is_recording_) return CapResObserve;

  if (!playback_stream_.has_initial_camera()) {
    // Set initial camera fields if this is the first event in the stream.
    Camera::WriteToProto(playback_stream_.mutable_initial_camera(), cam);
  } else if (latest_camera_.WorldWindow() != cam.WorldWindow() ||
             latest_camera_.ScreenDim() != cam.ScreenDim() ||
             latest_camera_.GetPPI() != cam.GetPPI()) {
    // Emit a camera_on_input event iff the camera has changed.
    Camera::WriteToProto(
        playback_stream_.add_events()->mutable_camera_on_input(), cam);
  }
  latest_camera_ = cam;

  SInput::WriteToProto(playback_stream_.add_events()->mutable_sinput(),
                       SInput(data, cam));

  return CapResObserve;
}

void InputRecorder::StartRecording() {
  is_recording_ = true;
  playback_stream_.Clear();
}

const proto::PlaybackStream& InputRecorder::StopRecording() {
  is_recording_ = false;
  return playback_stream_;
}

bool InputRecorder::IsRecording() const { return is_recording_; }

const proto::PlaybackStream& InputRecorder::PlaybackStream() const {
  return playback_stream_;
}

void InputRecorder::SetPlaybackStream(const proto::PlaybackStream& stream) {
  playback_stream_ = stream;
}

void InputRecorder::ClearPlaybackStream() { playback_stream_.Clear(); }

void InputRecorder::TakePlaybackStream(proto::PlaybackStream* dest) {
  *dest = std::move(playback_stream_);
  playback_stream_.Clear();
}

}  // namespace input
}  // namespace ink
