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

#include "ink/engine/util/dbg/current_thread.h"

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

static pthread_t GetThreadId() { return pthread_self(); }

CurrentThreadValidator::CurrentThreadValidator() : id_(GetThreadId()) {}

void CurrentThreadValidator::Reset() { id_ = GetThreadId(); }

void CurrentThreadValidator::CheckIfOnSameThread() const {
#if INK_DEBUG
  if (!pthread_equal(id_, GetThreadId())) {
    SLOG(SLOG_ERROR, "CurrentThreadValidator failed!");
  }
#endif
}
}  // namespace ink
