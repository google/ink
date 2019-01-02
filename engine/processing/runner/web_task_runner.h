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

#ifndef INK_ENGINE_PROCESSING_RUNNER_WEB_TASK_RUNNER_H_
#define INK_ENGINE_PROCESSING_RUNNER_WEB_TASK_RUNNER_H_

#if !defined(__asmjs__) && !defined(__wasm__)
#error "WebTaskRunner is only compatible with asm.js and WASM.";
#elif defined(__EMSCRIPTEN_PTHREADS__)
#error \
    "Compiling for WASM with thread-support; use AsyncTaskRunner instead " \
    "of WebTaskRunner."
#endif

#include <emscripten.h>
#include <memory>

#include "ink/engine/processing/runner/deferred_task_runner.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

class WebTaskRunner : public DeferredTaskRunner {
 public:
  using SharedDeps = service::Dependencies<FrameState>;

  explicit WebTaskRunner(std::shared_ptr<FrameState> frame_state)
      : DeferredTaskRunner(std::move(frame_state)) {}

 private:
  void RequestServicingOfTaskQueue() override {
    // If we've already registered a callback, we don't need to register another
    // one.
    if (has_requested_service_) return;

    emscripten_async_call(
        [](void* p) {
          auto* runner = static_cast<WebTaskRunner*>(p);
          runner->RunDeferredTasks();
          runner->has_requested_service_ = false;
        },
        this, -1);
    has_requested_service_ = true;
  }

  bool has_requested_service_ = false;
};

}  // namespace ink

#endif  // INK_ENGINE_PROCESSING_RUNNER_WEB_TASK_RUNNER_H_
