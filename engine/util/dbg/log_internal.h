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

#ifndef INK_ENGINE_UTIL_DBG_LOG_INTERNAL_H_
#define INK_ENGINE_UTIL_DBG_LOG_INTERNAL_H_

#ifdef USE_BASE_LOGGING
#include "base/logging.h"
#endif
#include <ostream>
#include <string>

#include "geo/render/ion/base/logging.h"
#include "ink/engine/util/dbg/log_levels.h"

// When using Ion logging we use LOG_PROD since we want to LOG_ERROR even in
// prod.
#ifdef ION_LOG_PROD
#define ACTUAL_LOG ION_LOG_PROD
#else
#define ACTUAL_LOG LOG
#endif

#define LOG_INTERNAL(LEVEL, MSG)     \
  if ((LEVEL)&SLOG_ERROR) {          \
    ACTUAL_LOG(ERROR) << (MSG);      \
  } else if ((LEVEL)&SLOG_WARNING) { \
    ACTUAL_LOG(WARNING) << (MSG);    \
  } else {                           \
    ACTUAL_LOG(INFO) << (MSG);       \
  }

#endif  // INK_ENGINE_UTIL_DBG_LOG_INTERNAL_H_
