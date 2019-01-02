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

#include "ink/engine/processing/runner/deterministic_task_runner.h"

namespace ink {
DeterministicTaskRunner::DeterministicTaskRunner(
    const service::UncheckedRegistry& registry)
    : DeterministicTaskRunner(registry.GetShared<FrameState>()) {}

DeterministicTaskRunner::DeterministicTaskRunner(
    std::shared_ptr<FrameState> frame_state)
    : frame_state_(frame_state) {}

void DeterministicTaskRunner::PushTask(std::unique_ptr<Task> task) {
  pending_.emplace_back(std::move(task));
  frame_state_->RequestFrameThreadSafe();
}

void DeterministicTaskRunner::ServiceMainThreadTasks() {
  for (auto& task : Flush()) {
    if (task->RequiresPreExecute()) task->PreExecute();
    task->Execute();
    task->OnPostExecute();
  }
}

std::deque<std::unique_ptr<Task>> DeterministicTaskRunner::Flush() {
  std::deque<std::unique_ptr<Task>> tasks = std::move(pending_);
  pending_.clear();
  return tasks;
}
}  // namespace ink
