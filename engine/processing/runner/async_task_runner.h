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

#ifndef INK_ENGINE_PROCESSING_RUNNER_ASYNC_TASK_RUNNER_H_
#define INK_ENGINE_PROCESSING_RUNNER_ASYNC_TASK_RUNNER_H_

#if (defined(__asmjs__) || defined(__wasm__)) && \
    !defined(__EMSCRIPTEN_PTHREADS__)
#error "AsyncTaskRunner is not compatible with asm.js or non-threaded WASM.";
#endif

#include <memory>
// Note this library uses standard C++11 thread support libraries.
#include <thread>

#include "third_party/absl/base/thread_annotations.h"
#include "third_party/absl/synchronization/mutex.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {

// A task runner that performs work on a separate worker thread. Each tasks'
// Execute() method is run on the worker thread, in the order that they were
// pushed to the queue. ServiceMainThreadTasks() calls OnPostExecute() for each
// task that has completed its Execute() method, in the same order -- this
// occurs on the main thread.
// Acquires a framerate lock when a task is pushed, and releases it in
// ServiceMainThreadTasks() when no tasks remain.
class AsyncTaskRunner : public ITaskRunner {
 public:
  using SharedDeps = service::Dependencies<FrameState>;

  explicit AsyncTaskRunner(const service::UncheckedRegistry& registry);
  explicit AsyncTaskRunner(std::shared_ptr<FrameState> frame_state);

  // Disallow copy and assign.
  AsyncTaskRunner(const AsyncTaskRunner&) = delete;
  AsyncTaskRunner& operator=(const AsyncTaskRunner&) = delete;

  ~AsyncTaskRunner() override;

  void PushTask(std::unique_ptr<Task> task) override;
  void ServiceMainThreadTasks() override;

  // The number of tasks that have been pushed by PushTask and not popped by
  // ServiceMainThreadTasks yet. Note that this may not be the same as
  // async_tasks_.size() + post_execute_tasks_.size(), as ThreadProc() and
  // ServiceMainThreadTasks() take ownership of a task before running Execute()
  // or PostExecute(), respectively.
  int NumPendingTasks() const { return num_pending_tasks_; }

 private:
  // Pops the foremost task off of the post-execution queue, taking ownership of
  // it. Returns absl::nullopt if the post-execution queue is empty.
  absl::optional<TaskWrapper> TakeNextPostExecuteTask();

  // This function indicates whether the worker thread may proceed with the
  // Execute() phase of the next task in the async queue.
  bool CanContinueAsyncExecution() const EXCLUSIVE_LOCKS_REQUIRED(mutex_);

  // Worker thread main procedure.
  void ThreadProc();

  absl::Mutex mutex_;
  std::thread worker_thread_;
  const std::shared_ptr<FrameState> frame_state_;
  std::unique_ptr<FramerateLock> framelock_;
  int num_pending_tasks_ = 0;

  // This indicates that the AsyncTaskRunner destructor has been called, and
  // that the worker thread should exit.
  bool should_exit_ GUARDED_BY(mutex_) = false;

  // This indicates that the worker thread is currently running the Execute()
  // phase of a task.
  bool is_executing_ GUARDED_BY(mutex_) = false;

  // Tasks queued for execution on the worker thread.
  std::queue<TaskWrapper> async_tasks_ GUARDED_BY(mutex_);

  // Tasks already executed on the worker thread and queued for post-execution.
  std::queue<TaskWrapper> post_execute_tasks_ GUARDED_BY(mutex_);
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_RUNNER_ASYNC_TASK_RUNNER_H_
