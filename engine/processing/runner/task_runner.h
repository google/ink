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

#ifndef INK_ENGINE_PROCESSING_RUNNER_TASK_RUNNER_H_
#define INK_ENGINE_PROCESSING_RUNNER_TASK_RUNNER_H_

#include <functional>
#include <memory>
#include <queue>

namespace ink {

// This class encapsulates some work that should be performed in the background.
class Task {
 public:
  virtual ~Task() {}

  // This indicates whether the task requires a PreExecute() phase.
  // WARNING: If this returns false, PreExecute() will not be called. If
  // PreExecute performs any work, this must return true.
  virtual bool RequiresPreExecute() const = 0;

  // This function may optionally be used to perform any work that must occur on
  // the main thread before the Execute() phase. This will be called only once
  // the previous task has finished its OnPostExecute() phase.
  // NOTE: Because PreExecute() cannot run until the previous task's
  // OnPostExecute() phase has completed, a task that requires a PreExecute()
  // phase will block the task queue. This means that only one task with a
  // PreExecute() phase can be completed for each call to
  // ServiceMainThreadTasks() (i.e. once per frame).
  // WARNING: If RequiresPreExecute() returns false, this will not be called. If
  // this performs any work, RequiresPreExecute() must return true.
  virtual void PreExecute() = 0;

  // This function may be used to perform work in the background. This may not
  // be called on the main thread, and as such should not modify anything in the
  // scene -- rather, it should save it results, and commit them in the
  // OnPostExecute() phase.
  virtual void Execute() = 0;

  // This function may be used to perform any work that must occur on the main
  // thread after the Execute() phase, such as committing the results from the
  // Execute() phase.
  virtual void OnPostExecute() = 0;
};

class LambdaTask : public Task {
 public:
  LambdaTask(std::function<void()> execute, std::function<void()> post_execute)
      : execute_(execute), post_execute_(post_execute) {}

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}

  void Execute() override {
    if (execute_) execute_();
  }

  void OnPostExecute() override {
    if (post_execute_) post_execute_();
  }

 private:
  std::function<void()> execute_;
  std::function<void()> post_execute_;
};

// A Task that only has a post_execute phase, to occur on the GL thread after
// flushing all tasks currently in queue.
class FlushTask : public LambdaTask {
 public:
  explicit FlushTask(std::function<void()> post_execute)
      : LambdaTask([]() {}, std::move(post_execute)) {}
};

// The task runner interface. Tasks may be pushed to the queue to perform work,
// via their Execute() method. The owner of the task runner is expected
// to periodically call ServiceMainThreadTasks(), which runs OnPostExecute()
// for each task that has completed its Execute() method, and PreExecute() for
// the next task if it requires it.
// Note that the task's Execute() method may be called on a different thread.
class ITaskRunner {
 public:
  virtual ~ITaskRunner() {}

  // Queues a task for execution.
  virtual void PushTask(std::unique_ptr<Task> task) = 0;

  // Runs OnPostExecute() on every task that has completed its Execute() method,
  // and PreExecute() for the next blocked task if necessary.
  virtual void ServiceMainThreadTasks() = 0;

 protected:
  // This convenience class is provided for subclasses. It wraps around a Task
  // to provide the ability to track whether it has completed its PreExecute()
  // phase.
  class TaskWrapper : public Task {
   public:
    explicit TaskWrapper(std::unique_ptr<Task> task) : task_(std::move(task)) {}

    bool RequiresPreExecute() const override {
      return task_->RequiresPreExecute();
    }
    void PreExecute() override {
      task_->PreExecute();
      is_pre_execute_complete_ = true;
    }
    void Execute() override { task_->Execute(); }
    void OnPostExecute() override { task_->OnPostExecute(); }

    bool IsReadyForExecutePhase() const {
      return !RequiresPreExecute() || is_pre_execute_complete_;
    }

   private:
    std::unique_ptr<Task> task_;
    bool is_pre_execute_complete_ = false;
  };
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_RUNNER_TASK_RUNNER_H_
