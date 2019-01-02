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

#ifndef INK_ENGINE_UTIL_TIME_WALL_CLOCK_H_
#define INK_ENGINE_UTIL_TIME_WALL_CLOCK_H_

#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class WallClockInterface {
 public:
  virtual ~WallClockInterface() {}

  virtual WallTimeS CurrentTime() const = 0;
};

class WallClock : public WallClockInterface {
 public:
  WallClock() : start_time_s_(SystemTime()) {}
  WallTimeS CurrentTime() const override;

 private:
  double SystemTime() const;
  double start_time_s_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_TIME_WALL_CLOCK_H_
