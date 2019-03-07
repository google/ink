/*
 * Copyright 2019 Google LLC
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

#ifndef INK_ENGINE_PROCESSING_BLOCKER_MANAGER_H_
#define INK_ENGINE_PROCESSING_BLOCKER_MANAGER_H_

#include <memory>
#include "third_party/absl/memory/memory.h"
#include "ink/engine/public/host/iengine_listener.h"
#include "ink/engine/service/dependencies.h"

namespace ink {

class BlockerLock;

class BlockerManager {
 public:
  using SharedDeps = service::Dependencies<IEngineListener>;

  explicit BlockerManager(std::shared_ptr<IEngineListener> engine_listener)
      : lock_counter_(new LockCounter(std::move(engine_listener))) {}

  std::unique_ptr<BlockerLock> AcquireLock() {
    return absl::make_unique<BlockerLock>(lock_counter_);
  }

  bool IsBlocked() const { return lock_counter_->NumLocks() > 0; }

 private:
  class LockCounter {
   public:
    explicit LockCounter(std::shared_ptr<IEngineListener> engine_listener)
        : engine_listener_(engine_listener) {}

    void Increment() {
      if (num_locks_ == 0) engine_listener_->BlockingStateChanged(true);
      ++num_locks_;
    }

    void Decrement() {
      --num_locks_;
      if (num_locks_ == 0) engine_listener_->BlockingStateChanged(false);
    }

    int NumLocks() const { return num_locks_; }

   private:
    std::shared_ptr<IEngineListener> engine_listener_;
    int num_locks_ = 0;
  };

  std::shared_ptr<LockCounter> lock_counter_;

  friend class BlockerLock;
};

class BlockerLock {
 public:
  explicit BlockerLock(
      std::shared_ptr<BlockerManager::LockCounter> lock_counter)
      : lock_counter_(std::move(lock_counter)) {
    lock_counter_->Increment();
  }
  ~BlockerLock() { lock_counter_->Decrement(); }

  BlockerLock(const BlockerLock&) = delete;
  BlockerLock& operator=(const BlockerLock&) = delete;
  BlockerLock(BlockerLock&&) = delete;
  BlockerLock& operator=(BlockerLock&&) = delete;

 private:
  std::shared_ptr<BlockerManager::LockCounter> lock_counter_;
};

}  // namespace ink

#endif  // INK_ENGINE_PROCESSING_BLOCKER_MANAGER_H_
