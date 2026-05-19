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

#ifndef INK_JNI_INTERNAL_JNI_BUFFER_UTIL_H_
#define INK_JNI_INTERNAL_JNI_BUFFER_UTIL_H_

#include <jni.h>

#include <cstddef>

#include "absl/base/nullability.h"
#include "absl/types/span.h"

namespace ink::jni {

// Converts a span of bytes to a Java direct ByteBuffer. Returns nullptr
// if the span is empty because the JNI interface does not provide a good way
// to allocate an empty direct byte-buffer. (When a span is empty, its data
// pointer may be nullptr, which NewDirectByteBuffer does not permit even with
// size 0.)
absl_nullable jobject ByteSpanToUnsafelyMutableDirectByteBuffer(
    JNIEnv* env, absl::Span<const std::byte> span);

}  // namespace ink::jni

#endif  // INK_JNI_INTERNAL_JNI_BUFFER_UTIL_H_
