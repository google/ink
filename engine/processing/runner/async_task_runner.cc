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

#include "ink/engine/processing/runner/async_task_runner.h"

#include "third_party/absl/types/optional.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

AsyncTaskRunner::AsyncTaskRunner(const service::UncheckedRegistry& registry)
    : AsyncTaskRunner(registry.GetShared<FrameState>()) {}

AsyncTaskRunner::AsyncTaskRunner(std::shared_ptr<FrameState> frame_state)
    : frame_state_(std::move(frame_state)) {
  worker_thread_ = std::thread(&AsyncTaskRunner::ThreadProc, this);
}

AsyncTaskRunner::~AsyncTaskRunner() {
  SLOG(SLOG_OBJ_LIFETIME, "workqueue dtor");
  {
    absl::MutexLock scoped_lock(&mutex_);
    should_exit_ = true;
  }
  worker_thread_.join();
}

void AsyncTaskRunner::PushTask(std::unique_ptr<Task> task) {
#if INK_DEBUG
  {
    absl::MutexLock scoped_lock(&mutex_);
    ASSERT(!should_exit_);
  }
#endif

  TaskWrapper wrapper(std::move(task));
  if (num_pending_tasks_ == 0 && !wrapper.IsReadyForExecutePhase()) {
    wrapper.PreExecute();
  }

  {
    absl::MutexLock lock(&mutex_);
    async_tasks_.push(std::move(wrapper));
  }
  num_pending_tasks_++;
  framelock_ =
      frame_state_->AcquireFramerateLock(30, "task runner pushing a task");
}

void AsyncTaskRunner::ServiceMainThreadTasks() {
  while (auto task = TakeNextPostExecuteTask()) {
    task.value().OnPostExecute();
    num_pending_tasks_--;
  }

  if (num_pending_tasks_ == 0) {
    framelock_.reset();
  } else {
    absl::MutexLock lock(&mutex_);
    // If there's nothing currently executing on the background thread, and
    // nothing in the post-execution queue, we can run PreExecute() on the
    // next task. Note that we need to check the post-execution queue, even
    // though we just emptied it, because a task may have completed its Execute
    // phase and been asynchronously pushed to the post-execute queue since
    // then.
    if (!is_executing_ && post_execute_tasks_.empty() &&
        !async_tasks_.empty() && !async_tasks_.front().IsReadyForExecutePhase())
      async_tasks_.front().PreExecute();
  }
}

absl::optional<ITaskRunner::TaskWrapper>
AsyncTaskRunner::TakeNextPostExecuteTask() {
  absl::optional<TaskWrapper> task;

  absl::MutexLock scoped_lock(&mutex_);
  if (!post_execute_tasks_.empty()) {
    task = std::move(post_execute_tasks_.front());
    post_execute_tasks_.pop();
  }

  return task;
}

void AsyncTaskRunner::ThreadProc() {
  while (true) {
    // Block until either should_exit_ is true, or the front task in
    // async_queue_ is ready for execution.
    mutex_.LockWhen(
        absl::Condition(this, &AsyncTaskRunner::CanContinueAsyncExecution));
    if (should_exit_) {
      mutex_.Unlock();
      break;
    }

    // If we're here, should_exit_ must be false, so async_queue_ must have a
    // task ready for execution.
    ASSERT(!async_tasks_.empty());
    auto task = std::move(async_tasks_.front());
    async_tasks_.pop();
    is_executing_ = true;
    mutex_.Unlock();

    task.Execute();

    absl::MutexLock lock(&mutex_);
    is_executing_ = false;
    post_execute_tasks_.push(std::move(task));
  }

  SLOG(SLOG_OBJ_LIFETIME, "taskrunner thread exit");
}

bool AsyncTaskRunner::CanContinueAsyncExecution() const {
  return should_exit_ || (!async_tasks_.empty() &&
                          async_tasks_.front().IsReadyForExecutePhase());
}

}  // namespace ink
