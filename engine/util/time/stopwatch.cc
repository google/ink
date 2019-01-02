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

#include "ink/engine/util/time/stopwatch.h"
#include <utility>

#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

Stopwatch::Stopwatch(std::shared_ptr<WallClockInterface> clock, bool start)
    : clock_(std::move(clock)),
      is_running_(start),
      elapsed_(0),
      start_time_(clock_->CurrentTime()) {}

DurationS Stopwatch::Elapsed() const {
  if (!is_running_) return elapsed_;
  return elapsed_ + clock_->CurrentTime() - start_time_;
}

void Stopwatch::Pause() {
  if (is_running_) {
    is_running_ = false;
    elapsed_ += (clock_->CurrentTime() - start_time_);
  }
}

void Stopwatch::Resume() {
  if (!is_running_) {
    is_running_ = true;
    start_time_ = clock_->CurrentTime();
  }
}

void Stopwatch::Reset() {
  elapsed_ = 0;
  is_running_ = false;
}

void Stopwatch::Restart() {
  start_time_ = clock_->CurrentTime();
  elapsed_ = 0;
  is_running_ = true;
}

}  // namespace ink
