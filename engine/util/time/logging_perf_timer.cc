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

#include "ink/engine/util/time/logging_perf_timer.h"

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

namespace {
constexpr bool kMeasurePerf = false;
constexpr DurationS kLogInterval(.5);
}  // namespace

LoggingPerfTimer::LoggingPerfTimer(std::shared_ptr<WallClockInterface> clock,
                                   std::string msg)
    : clock_(std::move(clock)), msg_(msg), last_log_time_(0), start_time_(0) {}

void LoggingPerfTimer::Begin() {
  if (kMeasurePerf) start_time_ = clock_->CurrentTime();
}

void LoggingPerfTimer::End() {
  if (kMeasurePerf) {
    WallTimeS current_time = clock_->CurrentTime();
    average_time_.Sample(current_time - start_time_);

    if (current_time > last_log_time_ + kLogInterval) {
      SLOG(SLOG_PERF, "$0: $1f ms\n", msg_,
           static_cast<double>(average_time_.Value()) * 1000.0);
      last_log_time_ = current_time;
    }
  }
}

}  // namespace ink
