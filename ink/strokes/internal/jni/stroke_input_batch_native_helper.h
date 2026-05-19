// Copyright 2024 Google LLC
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

#ifndef INK_STROKES_INTERNAL_JNI_STROKE_INPUT_BATCH_NATIVE_HELPER_H_
#define INK_STROKES_INTERNAL_JNI_STROKE_INPUT_BATCH_NATIVE_HELPER_H_

#include <cstdint>

#include "absl/log/absl_check.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace ink::native {

// Creates a new heap-allocated copy of the `StrokeInputBatch` and returns a
// pointer to it as a jlong, suitable for wrapping in a Kotlin
// StrokeInputBatch or MutableStrokeInputBatch.
inline int64_t NewNativeStrokeInputBatch(const StrokeInputBatch& batch) {
  return reinterpret_cast<int64_t>(new StrokeInputBatch(batch));
}

// Creates a new heap-allocated empty `StrokeInputBatch` and returns a
// pointer to it as a jlong, suitable for wrapping in a Kotlin
// StrokeInputBatch or MutableStrokeInputBatch.
inline int64_t NewNativeStrokeInputBatch() {
  return NewNativeStrokeInputBatch(StrokeInputBatch());
}

// Casts a Kotlin StrokeInputBatch.nativePointer to a C++ StrokeInputBatch.
// The returned StrokeInputBatch is a const ref as the kotlin StrokeInputBatch
// is immutable.
inline const StrokeInputBatch& CastToStrokeInputBatch(
    int64_t batch_native_pointer) {
  ABSL_CHECK_NE(batch_native_pointer, 0)
      << "Invalid native pointer for StrokeInputBatch.";
  return *reinterpret_cast<StrokeInputBatch*>(batch_native_pointer);
}

// Casts a Kotlin MutableStrokeInputBatch.nativePointer to a mutable C++
// StrokeInputBatch.
inline StrokeInputBatch& CastToMutableStrokeInputBatch(
    int64_t mutable_batch_native_pointer) {
  ABSL_CHECK_NE(mutable_batch_native_pointer, 0)
      << "Invalid native pointer for MutableStrokeInputBatch.";
  return *reinterpret_cast<StrokeInputBatch*>(mutable_batch_native_pointer);
}

// Frees a Kotlin StrokeInputBatch.nativePointer or
// MutableStrokeInputBatch.nativePointer.
inline void DeleteNativeStrokeInputBatch(int64_t native_pointer) {
  if (native_pointer == 0) return;
  delete reinterpret_cast<StrokeInputBatch*>(native_pointer);
}

}  // namespace ink::native

#endif  // INK_STROKES_INTERNAL_JNI_STROKE_INPUT_BATCH_NATIVE_HELPER_H_
