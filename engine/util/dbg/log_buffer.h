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

#ifndef INK_ENGINE_UTIL_DBG_LOG_BUFFER_H_
#define INK_ENGINE_UTIL_DBG_LOG_BUFFER_H_

#include <string>
#include <vector>

#include "geo/render/ion/base/logging.h"
#include "third_party/absl/strings/str_format.h"
#include "third_party/absl/synchronization/mutex.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/wall_clock.h"

#define ION_LOG_WITH_FILE(severity, file, line)                            \
  ::ion::base::logging_internal::Logger(file, line, ::ion::port::severity) \
      .GetStream()

#define LOG_INTERNAL_WITH_FILE(level, file, line, msg) \
  if ((level)&SLOG_ERROR) {                            \
    ION_LOG_WITH_FILE(ERROR, file, line) << (msg);     \
  } else if ((level)&SLOG_WARNING) {                   \
    ION_LOG_WITH_FILE(WARNING, file, line) << (msg);   \
  } else {                                             \
    ION_LOG_WITH_FILE(INFO, file, line) << (msg);      \
  }

// Same usage as SLOG(), but won't actually write to the log until SLOG_FLUSH()
// is called.
// For debug use only.
#define SLOG_LATER(level, fmt, ...)                                \
  do {                                                             \
    absl::MutexLock lock(::ink::LogBuffer::mutex_);                \
    ::ink::LogBuffer::logs_->push_back(::ink::LogBuffer::Entry(    \
        {level, ::ink::Substitute((fmt), ##__VA_ARGS__), __FILE__, \
         __LINE__}));                                              \
  } while (false);

// Log the message along with a timestamp delta since the first logged message
// and a delta since the last timestamped log..
// For debug use only.
#define SLOG_LATER_WITH_TIME(level, fmt, ...)                              \
  do {                                                                     \
    absl::MutexLock lock(::ink::LogBuffer::mutex_);                        \
    double logtime =                                                       \
        static_cast<double>(::ink::LogBuffer::clock_->CurrentTime());      \
    if (::ink::LogBuffer::first_time_ < 0) {                               \
      ::ink::LogBuffer::first_time_ = logtime;                             \
    }                                                                      \
    double delta = (::ink::LogBuffer::last_time_ < 0)                      \
                       ? 0                                                 \
                       : logtime - ::ink::LogBuffer::last_time_;           \
    ::ink::LogBuffer::last_time_ = logtime;                                \
    auto fmt_with_time = absl::StrCat(                                     \
        (fmt),                                                             \
        absl::StrFormat(" @%.1f ms [+%.1f ms]",                            \
                        1000 * (logtime - ::ink::LogBuffer::first_time_),  \
                        1000 * delta));                                    \
    ::ink::LogBuffer::logs_->push_back(::ink::LogBuffer::Entry(            \
        {level, ::ink::Substitute(fmt_with_time, ##__VA_ARGS__), __FILE__, \
         __LINE__}));                                                      \
  } while (false);

// Write out all the stored logs using ion then clear the logs.
// For debug use only.
#define SLOG_FLUSH() ::ink::LogBuffer::Flush();

namespace ink {

// LogBuffer stores logs until flushing them to output. Use the above macros
// instead of interacting with this directly.
class LogBuffer {
 public:
  struct Entry {
    int level;
    string log;
    string file;
    int line;
  };
  static void Flush();

  static absl::Mutex* mutex_;
  static std::vector<Entry>* logs_ GUARDED_BY(mutex_);
  static double last_time_ GUARDED_BY(mutex_);
  static double first_time_ GUARDED_BY(mutex_);
  static WallClock* clock_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_DBG_LOG_BUFFER_H_
