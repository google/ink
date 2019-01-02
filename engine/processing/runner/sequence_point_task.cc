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

#include "ink/engine/processing/runner/sequence_point_task.h"

namespace ink {

SequencePointTask::SequencePointTask(int32_t id,
                                     std::shared_ptr<FrameState> frame_state)
    : id_(id), weak_frame_state_(frame_state) {}

void SequencePointTask::OnPostExecute() {
  // Send the sequence point to the FrameState to be dispatched when the frame
  // is done.
  auto frame_state = weak_frame_state_.lock();
  if (frame_state) frame_state->SequencePointReached(id_);
}
}  // namespace ink
