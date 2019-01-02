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

#ifndef INK_ENGINE_INPUT_INPUT_COALESCER_H_
#define INK_ENGINE_INPUT_INPUT_COALESCER_H_

// ░█▀▄░█▀█░░░█▀█░█▀█░▀█▀░░░█░█░█▀▀░█▀▀░░░▀█▀░█░█░▀█▀░█▀▀░░░█▀▀░▀█▀░█░░░█▀▀
// ░█░█░█░█░░░█░█░█░█░░█░░░░█░█░▀▀█░█▀▀░░░░█░░█▀█░░█░░▀▀█░░░█▀▀░░█░░█░░░█▀▀
// ░▀▀░░▀▀▀░░░▀░▀░▀▀▀░░▀░░░░▀▀▀░▀▀▀░▀▀▀░░░░▀░░▀░▀░▀▀▀░▀▀▀░░░▀░░░▀▀▀░▀▀▀░▀▀▀
//
// WARNING: this file was re-added to the repository because of the Wear
// Handwriting project. You probably don't want to use it (and we will
// hopefully re-delete it in the future.) See b/29177172 for more context.

#if WEAR_HANDWRITING

#include <cstdint>
#include <deque>
#include <map>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
namespace ink {
namespace input {

class InputDispatch;

// Performs input coalescing (merging consecutive moves) on a per-id basis
class InputCoalescer {
 public:
  InputCoalescer() {}
  void QueueInput(InputDispatch* dispatch, const Camera& cam, InputData data);
  void DispatchQueuedInput(InputDispatch* dispatch, const Camera& cam);

 private:
  bool PopMinTimeData(InputData* res);

 private:
  std::map<uint32_t, std::deque<InputData>> id_to_data_;
};
}  // namespace input
}  // namespace ink

#endif  // WEAR_HANDWRITING

#endif  // INK_ENGINE_INPUT_INPUT_COALESCER_H_
