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

#ifndef INK_ENGINE_UTIL_TIME_STOPWATCH_H_
#define INK_ENGINE_UTIL_TIME_STOPWATCH_H_

#include <memory>

#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

class Stopwatch {
 public:
  explicit Stopwatch(std::shared_ptr<WallClockInterface> clock,
                     bool start = true);

  DurationS Elapsed() const;

  void Pause();
  void Resume();
  void Reset();
  void Restart();

 private:
  std::shared_ptr<WallClockInterface> clock_;
  bool is_running_;
  DurationS elapsed_;
  WallTimeS start_time_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_TIME_STOPWATCH_H_
