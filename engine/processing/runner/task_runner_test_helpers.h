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

#ifndef INK_ENGINE_PROCESSING_RUNNER_TASK_RUNNER_TEST_HELPERS_H_
#define INK_ENGINE_PROCESSING_RUNNER_TASK_RUNNER_TEST_HELPERS_H_

#include <thread>

#include "third_party/absl/base/thread_annotations.h"
#include "third_party/absl/synchronization/mutex.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

class ThreadSafeCounter {
 public:
  int Next() {
    absl::MutexLock lock(&mutex_);
    return next_++;
  }

 private:
  absl::Mutex mutex_;
  int next_ GUARDED_BY(mutex_) = 0;
};

// This indicates how far a task has gotten in its execution.
enum class TaskProgress {
  kNotStarted = 0,
  kPreExecuteComplete = 1,
  kExecuteComplete = 2,
  kPostExecuteComplete = 3,
};

// This contains information on when, and on which thread, each of the task's
// methods were executed.
class TaskState {
 public:
  static constexpr int kNotExecuted = -1;

  // Returns true if the stored information represents a valid task state, i.e.
  // that the relative order that the task's methods were run is correct.
  bool IsValid() const;

  // Prevents the associated TestTask from completing its Execute() method until
  // UnblockExecutionAndWait() is called. Note that this must be called before
  // Execute() begins, so it should be done before the task is pushed to the
  // runner. If UnblockExecutionAndWait() is never called, the task could block
  // indefinitely, resulting in a test timeout. This should only be used for
  // task runner implementations that run asynchronously.
  void BlockExecution() {
    absl::MutexLock lock(&mutex_);
    allow_execution_ = false;
  }

  // Blocks until either the task has completed its Execute() method, or the
  // given timeout has expired. Returns true if Execute() completed. This should
  // only be used for task runner implementations that run asynchronously.
  bool UnblockExecutionAndWait(absl::Duration timeout = absl::Seconds(2));

  TaskProgress GetProgress() const;

  bool WasTaskDestroyed() const {
    absl::MutexLock lock(&mutex_);
    return dtor_order_ != kNotExecuted;
  }

  int PreExecuteOrder() const {
    absl::MutexLock lock(&mutex_);
    return pre_execute_order_;
  }
  int ExecuteOrder() const {
    absl::MutexLock lock(&mutex_);
    return execute_order_;
  }
  int PostExecuteOrder() const {
    absl::MutexLock lock(&mutex_);
    return post_execute_order_;
  }
  int DtorOrder() const {
    absl::MutexLock lock(&mutex_);
    return dtor_order_;
  }

  std::thread::id PreExecuteThreadId() const {
    absl::MutexLock lock(&mutex_);
    return pre_execute_thread_id_;
  }
  std::thread::id ExecuteThreadId() const {
    absl::MutexLock lock(&mutex_);
    return execute_thread_id_;
  }
  std::thread::id PostExecuteThreadId() const {
    absl::MutexLock lock(&mutex_);
    return post_execute_thread_id_;
  }
  std::thread::id DtorThreadId() const {
    absl::MutexLock lock(&mutex_);
    return dtor_thread_id_;
  }

 private:
  mutable absl::Mutex mutex_;

  bool allow_execution_ GUARDED_BY(mutex_) = true;

  int pre_execute_order_ GUARDED_BY(mutex_) = kNotExecuted;
  int execute_order_ GUARDED_BY(mutex_) = kNotExecuted;
  int post_execute_order_ GUARDED_BY(mutex_) = kNotExecuted;
  int dtor_order_ GUARDED_BY(mutex_) = kNotExecuted;

  std::thread::id pre_execute_thread_id_ GUARDED_BY(mutex_);
  std::thread::id execute_thread_id_ GUARDED_BY(mutex_);
  std::thread::id post_execute_thread_id_ GUARDED_BY(mutex_);
  std::thread::id dtor_thread_id_ GUARDED_BY(mutex_);

  bool requires_pre_execute_ = false;

  friend class TestTask;
};

// This task updates the information in a state object in each of its methods.
// The state object outlives the task, allowing us to examine the relevant
// information after the task has been destroyed.
class TestTask : public Task {
 public:
  TestTask(TaskState* state, ThreadSafeCounter* counter,
           bool requires_pre_execute)
      : state_(state), counter_(counter) {
    state_->requires_pre_execute_ = requires_pre_execute;
  }

  ~TestTask() override {
    absl::MutexLock lock(&state_->mutex_);
    state_->dtor_thread_id_ = std::this_thread::get_id();
    state_->dtor_order_ = counter_->Next();
  }

  bool RequiresPreExecute() const override {
    return state_->requires_pre_execute_;
  }

  void PreExecute() override {
    absl::MutexLock lock(&state_->mutex_);
    ASSERT(RequiresPreExecute());
    state_->pre_execute_thread_id_ = std::this_thread::get_id();
    state_->pre_execute_order_ = counter_->Next();
  }

  void Execute() override {
    // This will block until the mutex can be acquired and allow_execution_ is
    // true.
    state_->mutex_.LockWhen(absl::Condition(&state_->allow_execution_));

    state_->execute_thread_id_ = std::this_thread::get_id();
    state_->execute_order_ = counter_->Next();
    state_->mutex_.Unlock();
  }

  void OnPostExecute() override {
    absl::MutexLock lock(&state_->mutex_);
    state_->post_execute_thread_id_ = std::this_thread::get_id();
    state_->post_execute_order_ = counter_->Next();
  }

 private:
  TaskState* state_;
  ThreadSafeCounter* counter_;
};

}  // namespace ink

#endif  // INK_ENGINE_PROCESSING_RUNNER_TASK_RUNNER_TEST_HELPERS_H_
