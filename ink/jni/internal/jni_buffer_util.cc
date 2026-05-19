// Copyright 2026 Google LLC
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

#include "ink/jni/internal/jni_buffer_util.h"

#include <jni.h>

#include <cstddef>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/types/span.h"

namespace ink::jni {

absl_nullable jobject ByteSpanToUnsafelyMutableDirectByteBuffer(
    JNIEnv* env, absl::Span<const std::byte> span) {
  // span.data() may be nullptr if empty, which NewDirectByteBuffer does not
  // permit.
  if (span.empty()) return nullptr;
  ABSL_CHECK(span.data() != nullptr);
  return env->NewDirectByteBuffer(
      // NewDirectByteBuffer needs a non-const void*. The resulting buffer is
      // writeable, so it must be wrapped at the Kotlin layer in a read-only
      // buffer that delegates to this one.
      const_cast<std::byte*>(span.data()), span.size());
}

}  // namespace ink::jni
