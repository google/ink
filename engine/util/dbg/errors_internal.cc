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

#include "ink/engine/util/dbg/errors_internal.h"

#include <cassert>
#include <cstdint>

#include "ink/engine/public/host/exit.h"
#include "ink/engine/util/dbg/log_internal.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

void Die(const std::string& msg) {
  LOG_INTERNAL(SLOG_ERROR, msg);
#if !(defined(__native_client__) || defined(__asmjs__) || defined(__wasm__))
  // Don't crash the tab on web.
  assert(false);
#endif
  ink::exit();
}
}  // namespace ink
