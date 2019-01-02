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

// Sketchology logging.
//
// All logging should be done throw the SLOG macro, provided here.
//
// A call to SLOG looks like
//
//    SLOG(SLOG_INFO, "x: $0, n: $1", my_thing, my_other_thing);
//
// The first parameter is one of the log "levels" provided in log_levels.h.
// The second parameter is a literal string with N absl::Substitute-style field
// specs. The remaining N parameters are converted to strings per str.h, in this
// directory.
//
#ifndef INK_ENGINE_UTIL_DBG_LOG_H_
#define INK_ENGINE_UTIL_DBG_LOG_H_

#include <cstdint>
#include <iterator>
#include <sstream>
#include <string>

#include "ink/engine/util/dbg/log_internal.h"
#include "ink/engine/util/dbg/log_levels.h"  // IWYU pragma: export
#include "ink/engine/util/dbg/str.h"

namespace ink {

#if !INK_DEBUG
static constexpr uint32_t default_log_level =
    (SLOG_ERROR |
     SLOG_WARNING
// Log packet correction for everything but Desktop Linux.
// Android is a subset of Linux, so we make sure to check it explicitly.
#if !defined(__linux__) || defined(__ANDROID__)
     //  Don't log packet correction on linux - the input stream there does not
     // look like the other platforms.
     | SLOG_INPUT_CORRECTION
#endif  // !INK_PLATFORM_LINUX
    );
#else
static constexpr uint32_t default_log_level =
    (SLOG_ERROR | SLOG_WARNING |
     SLOG_INFO
#if !defined(__linux__) || defined(__ANDROID__)
     //  Don't log packet correction on linux - the input stream there does not
     // look like the other platforms.
     | SLOG_INPUT_CORRECTION
#endif  // !INK_PLATFORM_LINUX
     // | SLOG_TEXTURES
     // | SLOG_PDF
     // | SLOG_PERF
     // | SLOG_DATA_FLOW
     // | SLOG_DRAWING
     // | SLOG_GPU_OBJ_CREATION
     // | SLOG_GL_STATE
     // | SLOG_FRAMERATE_LOCKS
     // | SLOG_FILE_IO
     // | SLOG_OBJ_LIFETIME
     // | SLOG_LINE_PROCESSING
     // | SLOG_INPUT
     // | SLOG_MODIFIERS
     // | SLOG_ANIMATION
     // | SLOG_CAMERA
     // | SLOG_JNI_TRACE
     // | SLOG_TOOLS
     // | SLOG_DOCUMENT
     // | SLOG_BOUNDS
     // | SLOG_BOOLEAN_OPERATION
    );
#endif  // !INK_DEBUG

constexpr bool CheckLevel(uint32_t level) { return level & default_log_level; }

// http://www.parashift.com/c++-faq-lite/macros-with-multi-stmts.html
#define SLOG(level, fmt, ...)                                       \
  do {                                                              \
    if (::ink::CheckLevel(level)) {                                 \
      LOG_INTERNAL(level, ::ink::Substitute((fmt), ##__VA_ARGS__)); \
    }                                                               \
  } while (false)

}  // namespace ink
#endif  // INK_ENGINE_UTIL_DBG_LOG_H_
