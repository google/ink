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

#ifndef INK_ENGINE_INPUT_INPUT_RECORDER_H_
#define INK_ENGINE_INPUT_INPUT_RECORDER_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {
namespace input {

class InputRecorder : public InputHandler {
 public:
  explicit InputRecorder(const service::UncheckedRegistry& registry);
  explicit InputRecorder(std::shared_ptr<InputDispatch> dispatch);
  CaptureResult OnInput(const InputData& data, const Camera& camera) override;

  void StartRecording();
  const proto::PlaybackStream& StopRecording();
  bool IsRecording() const;
  const proto::PlaybackStream& PlaybackStream() const;

  // Copies stream into the recorder's internal buffer, overwriting whatever may
  // already be there. Does not change whether we are currently recording. It is
  // the responsibility of the caller to avoid races between incoming input and
  // the existing buffer being clobbered.
  void SetPlaybackStream(const proto::PlaybackStream& stream);

  // Drops all recordered content. Does not change whether we are currently
  // recording.
  void ClearPlaybackStream();

  // Moves out of the encapsulated stream into dest. After this method
  // returns, internally buffered playback events will have been
  // cleared, and this recorder may be used again.
  void TakePlaybackStream(proto::PlaybackStream* dest);

  inline std::string ToString() const override { return "<InputRecorder>"; }

 private:
  bool is_recording_ = false;
  Camera latest_camera_;
  proto::PlaybackStream playback_stream_;
};

}  // namespace input
}  // namespace ink
#endif  // INK_ENGINE_INPUT_INPUT_RECORDER_H_
