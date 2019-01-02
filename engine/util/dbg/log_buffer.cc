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

#include "ink/engine/util/dbg/log_buffer.h"

namespace ink {

std::vector<LogBuffer::Entry>* LogBuffer::logs_ =
    new std::vector<LogBuffer::Entry>();
double LogBuffer::last_time_ = -1;
double LogBuffer::first_time_ = -1;
absl::Mutex* LogBuffer::mutex_ = new Mutex();
WallClock* LogBuffer::clock_ = new WallClock();

void LogBuffer::Flush() {
  absl::MutexLock lock(mutex_);
  for (auto l : *LogBuffer::logs_) {
    // Log just the filename, not the full path, to save space.
    LOG_INTERNAL_WITH_FILE(l.level, strrchr(l.file.c_str(), '/') + 1, l.line,
                           l.log);
  }
  LogBuffer::logs_->clear();
  LogBuffer::first_time_ = -1;
  LogBuffer::last_time_ = -1;
}

}  // namespace ink
