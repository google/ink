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

#include "ink/engine/util/time/wall_clock.h"

#include <sys/time.h>
#include <ctime>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/time/time_types.h"

#ifdef __APPLE__
#import <QuartzCore/CABase.h>
#endif

namespace ink {

WallTimeS WallClock::CurrentTime() const {
  return WallTimeS(SystemTime() - start_time_s_ + 0x100000000);
}

double WallClock::SystemTime() const {
#ifdef __APPLE__
  return CACurrentMediaTime();
#else
  timespec ts;
  EXPECT(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
  return ts.tv_sec + ts.tv_nsec / 1000000000.0;
#endif
}

}  // namespace ink
