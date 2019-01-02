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

#ifndef INK_ENGINE_UTIL_DBG_LOG_LEVELS_H_
#define INK_ENGINE_UTIL_DBG_LOG_LEVELS_H_

namespace ink {

// clang-format off
#define SLOG_ERROR             (1 << 0)
#define SLOG_WARNING           (1 << 1)
#define SLOG_DATA_FLOW         (1 << 2)
#define SLOG_DRAWING           (1 << 3)
#define SLOG_PERF              (1 << 4)
#define SLOG_GPU_OBJ_CREATION  (1 << 5)
#define SLOG_GL_STATE          (1 << 6)
#define SLOG_FRAMERATE_LOCKS   (1 << 7)
#define SLOG_FILE_IO           (1 << 8)
#define SLOG_OBJ_LIFETIME      (1 << 9)
#define SLOG_LINE_PROCESSING   (1 << 10)
#define SLOG_INFO              (1 << 11)
#define SLOG_INPUT             (1 << 12)
#define SLOG_MODIFIERS         (1 << 13)
#define SLOG_ANIMATION         (1 << 14)
#define SLOG_CAMERA            (1 << 15)
#define SLOG_JNI_TRACE         (1 << 16)
#define SLOG_TOOLS             (1 << 17)
#define SLOG_DOCUMENT          (1 << 18)
#define SLOG_BOUNDS            (1 << 19)
#define SLOG_PDF               (1 << 20)
#define SLOG_INPUT_CORRECTION  (1 << 21)
#define SLOG_BOOLEAN_OPERATION (1 << 22)
#define SLOG_TEXTURES          (1 << 23)
// clang-format on

}  // namespace ink
#endif  // INK_ENGINE_UTIL_DBG_LOG_LEVELS_H_
