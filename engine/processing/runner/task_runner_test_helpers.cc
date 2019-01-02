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

#include "ink/engine/processing/runner/task_runner_test_helpers.h"

namespace ink {

TaskProgress TaskState::GetProgress() const {
  absl::MutexLock lock(&mutex_);

  if (post_execute_order_ != kNotExecuted) {
    return TaskProgress::kPostExecuteComplete;
  } else if (execute_order_ != kNotExecuted) {
    return TaskProgress::kExecuteComplete;
  } else if (pre_execute_order_ != kNotExecuted) {
    return TaskProgress::kPreExecuteComplete;
  } else {
    return TaskProgress::kNotStarted;
  }
}

bool TaskState::UnblockExecutionAndWait(absl::Duration timeout) {
  mutex_.Lock();
  allow_execution_ = true;
  bool did_complete = mutex_.AwaitWithTimeout(
      absl::Condition(
          +[](int* execute_order_) { return *execute_order_ != kNotExecuted; },
          &execute_order_),
      timeout);
  mutex_.Unlock();
  return did_complete;
}

bool TaskState::IsValid() const {
  absl::MutexLock lock(&mutex_);

  // If the destructor was called, it should have occurred last. Note,
  // however, that the other functions may not have been called.
  if (dtor_order_ != kNotExecuted) {
    if (pre_execute_order_ == kNotExecuted &&
        pre_execute_order_ >= dtor_order_) {
      return false;
    }

    if (execute_order_ == kNotExecuted && execute_order_ >= dtor_order_) {
      return false;
    }

    if (post_execute_order_ == kNotExecuted &&
        post_execute_order_ >= dtor_order_) {
      return false;
    }
  }

  // If OnPostExecute() was called, Execute() should have been called
  // beforehand.
  if (post_execute_order_ != kNotExecuted &&
      (execute_order_ == kNotExecuted ||
       execute_order_ >= post_execute_order_)) {
    return false;
  }

  if (requires_pre_execute_) {
    // If OnPostExecute() was called, Execute() should have been called
    // beforehand.
    if (execute_order_ != kNotExecuted &&
        (pre_execute_order_ == kNotExecuted ||
         pre_execute_order_ >= execute_order_)) {
      return false;
    }
  } else if (pre_execute_order_ != kNotExecuted) {
    // PreExecute() was called, but it should not have been.
    return false;
  }

  return true;
}

}  // namespace ink
