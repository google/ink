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

#ifndef INK_ENGINE_PROCESSING_RUNNER_SEQUENCE_POINT_TASK_H_
#define INK_ENGINE_PROCESSING_RUNNER_SEQUENCE_POINT_TASK_H_

#include <cstdint>
#include <memory>

#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/scene/frame_state/frame_state.h"

namespace ink {

class SequencePointTask : public Task {
 public:
  SequencePointTask(int32_t id, std::shared_ptr<FrameState> frame_state);

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}
  void Execute() override {}
  void OnPostExecute() override;

 private:
  int32_t id_;
  std::weak_ptr<FrameState> weak_frame_state_;
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_RUNNER_SEQUENCE_POINT_TASK_H_
