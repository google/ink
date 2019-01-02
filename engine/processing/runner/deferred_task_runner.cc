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

#include "ink/engine/processing/runner/deferred_task_runner.h"

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

void DeferredTaskRunner::RunDeferredTasks() {
  // Note that, because this is run synchronously, we don't need to protect the
  // data members with a mutex.
  while (!deferred_tasks_.empty() &&
         deferred_tasks_.front().IsReadyForExecutePhase()) {
    auto task = std::move(deferred_tasks_.front());
    deferred_tasks_.pop();
    task.Execute();
    post_execute_tasks_.push(std::move(task));
  }
}

void DeferredTaskRunner::PushTask(std::unique_ptr<Task> task) {
  TaskWrapper wrapper(std::move(task));
  if (deferred_tasks_.empty()) {
    if (!wrapper.IsReadyForExecutePhase()) wrapper.PreExecute();

    // If no tasks are queued, request a callback to RunDeferredTasks(), and
    // acquire a framelock. All tasks accumulated before the callback will be
    // executed by RunDeferredTasks(), so there is no need to request an
    // additional callback if a request has already been made.
    RequestServicingOfTaskQueue();
    framelock_ =
        frame_state_->AcquireFramerateLock(30, "task runner pushing a task");
  }
  deferred_tasks_.push(std::move(wrapper));
}

void DeferredTaskRunner::ServiceMainThreadTasks() {
  while (!post_execute_tasks_.empty()) {
    auto task = std::move(post_execute_tasks_.front());
    post_execute_tasks_.pop();
    task.OnPostExecute();
  }

  if (deferred_tasks_.empty()) {
    framelock_.reset();
  } else if (!deferred_tasks_.front().IsReadyForExecutePhase()) {
    deferred_tasks_.front().PreExecute();
    RequestServicingOfTaskQueue();
  }
}

}  // namespace ink
