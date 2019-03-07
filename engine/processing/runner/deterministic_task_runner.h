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

#ifndef INK_ENGINE_PROCESSING_RUNNER_DETERMINISTIC_TASK_RUNNER_H_
#define INK_ENGINE_PROCESSING_RUNNER_DETERMINISTIC_TASK_RUNNER_H_

#include <deque>

#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {
// Completely deterministic task runner.
class DeterministicTaskRunner : public ITaskRunner {
 public:
  using SharedDeps = service::Dependencies<FrameState>;

  explicit DeterministicTaskRunner(const service::UncheckedRegistry& registry);
  explicit DeterministicTaskRunner(std::shared_ptr<FrameState> frame_state);

  // Enqueues task to be executed and returns immediately.
  void PushTask(std::unique_ptr<Task> task) override;

  // Runs PreExecute() (if required), Execute(), and OnPostExecute() on all
  // pending tasks. Blocks until they are all completed.
  void ServiceMainThreadTasks() override;

  int NumPendingTasks() const override { return pending_.size(); }

 private:
  // Returns the current items on the queue (and emptying it so that re-entrant
  // calls to PushTask can enqueue new items).
  std::deque<std::unique_ptr<Task>> Flush();

  std::deque<std::unique_ptr<Task>> pending_;
  std::shared_ptr<FrameState> frame_state_;
};
}  // namespace ink

#endif  // INK_ENGINE_PROCESSING_RUNNER_DETERMINISTIC_TASK_RUNNER_H_
