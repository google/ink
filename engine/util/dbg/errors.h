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

#ifndef INK_ENGINE_UTIL_DBG_ERRORS_H_
#define INK_ENGINE_UTIL_DBG_ERRORS_H_

#include <cstdlib>

#include "third_party/absl/strings/substitute.h"
#include "ink/engine/util/dbg/errors_internal.h"
#include "ink/engine/util/dbg/log_internal.h"
#include "ink/engine/util/dbg/str.h"

namespace ink {

// If !x, logs an error and triggers an assert.
// Enabled based on the NDEBUG flag (same as <cassert>).
#if !INK_DEBUG
#define ASSERT(x)
#else
#define ASSERT(x) (::ink::ExpectInternal((x), #x, __func__, __FILE__, __LINE__))
#endif

// If !x, logs an error and throws a runtime exception.
// Always enabled.
#define EXPECT(x) (::ink::ExpectInternal((x), #x, __func__, __FILE__, __LINE__))

// Logs an error and throws a runtime exception.
// Always enabled.
// Implemented as a macro to permit clang to see via static analysis that
// RUNTIME_ERROR always throws a runtime_error, and can therefore prevent
// a function from exiting without returning.
#define RUNTIME_ERROR(fmt, ...)                                                \
  do {                                                                         \
    /* Don't create a var for the result of createRuntimeErrorMsg to avoid */  \
    /* hiding locals at the macro insertion point */                           \
    ::ink::Die(::ink::CreateRuntimeErrorMsg(                                   \
        ::ink::Substitute(fmt, ##__VA_ARGS__), __func__, __FILE__, __LINE__)); \
                                                                               \
    /* This is not hit, die will always exit. However, escape analysis */      \
    /* can't prove it - so we add this. */                                     \
    std::exit(EXIT_FAILURE);                                                   \
  } while (0);

}  // namespace ink
#endif  // INK_ENGINE_UTIL_DBG_ERRORS_H_
