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

#ifndef INK_ENGINE_UTIL_DBG_ERRORS_INTERNAL_H_
#define INK_ENGINE_UTIL_DBG_ERRORS_INTERNAL_H_

#include <string>

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {

// Do not use these functions directly, use errors.h instead
inline std::string CreateRuntimeErrorMsg(absl::string_view message,
                                         absl::string_view func,
                                         absl::string_view path, int line) {
  const auto idx = path.find_last_of("\\/");
  const auto filename = path.substr(idx + 1);

  return Substitute("$0 in $1 at $2:$3", message, func, filename, line);
}

void Die(const std::string& msg);

template <typename T>
void ExpectInternal(T f, absl::string_view expr, const char* func,
                    const char* file, int line) {
  if (!f) {
    Die(CreateRuntimeErrorMsg(Substitute("expected $0", expr), func, file,
                              line));
  }
}

}  // namespace ink
#endif  // INK_ENGINE_UTIL_DBG_ERRORS_INTERNAL_H_
