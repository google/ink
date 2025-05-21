// Copyright 2024-2025 Google LLC
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

#ifndef INK_STROKES_INTERNAL_JNI_STROKE_INPUT_JNI_HELPER_H_
#define INK_STROKES_INTERNAL_JNI_STROKE_INPUT_JNI_HELPER_H_

#include <jni.h>

#include "absl/log/absl_check.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace ink::jni {

// Creates a new stack-allocated copy of the `StrokeInputBatch` and returns a
// pointer to it as a jlong, suitable for wrapping in a Kotlin
// StrokeInputBatch or MutableStrokeInputBatch.
inline jlong NewNativeStrokeInputBatch(const StrokeInputBatch& batch) {
  return reinterpret_cast<jlong>(new StrokeInputBatch(batch));
}

// Creates a new stack-allocated empty `StrokeInputBatch` and returns a
// pointer to it as a jlong, suitable for wrapping in a Kotlin
// StrokeInputBatch or MutableStrokeInputBatch.
inline jlong NewNativeStrokeInputBatch() {
  return NewNativeStrokeInputBatch(StrokeInputBatch());
}

// Casts a Kotlin StrokeInputBatch.nativePointer to a C++ StrokeInputBatch.
// The returned StrokeInputBatch is a const ref as the kotlin StrokeInputBatch
// is immutable.
inline const StrokeInputBatch& CastToStrokeInputBatch(
    jlong batch_native_pointer) {
  ABSL_CHECK_NE(batch_native_pointer, 0)
      << "Invalid native pointer for StrokeInputBatch.";
  return *reinterpret_cast<StrokeInputBatch*>(batch_native_pointer);
}

// Casts a Kotlin MutableStrokeInputBatch.nativePointer to a mutable C++
// StrokeInputBatch.
inline StrokeInputBatch& CastToMutableStrokeInputBatch(
    jlong mutable_batch_native_pointer) {
  ABSL_CHECK_NE(mutable_batch_native_pointer, 0)
      << "Invalid native pointer for MutableStrokeInputBatch.";
  return *reinterpret_cast<StrokeInputBatch*>(mutable_batch_native_pointer);
}

// Frees a Kotlin StrokeInputBatch.nativePointer or
// MutableStrokeInputBatch.nativePointer.
inline void DeleteNativeStrokeInputBatch(jlong native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0)
      << "Invalid native pointer for StrokeInputBatch.";
  delete reinterpret_cast<StrokeInputBatch*>(native_pointer);
}

// Converts Kotlin jint representation of InputToolType enum to C++
// StrokeInput::ToolType enum.
StrokeInput::ToolType JIntToToolType(jint val);

// Converts C++ StrokeInput::ToolType enum to Kotlin jint representation of
// InputToolType enum.
jint ToolTypeToJInt(StrokeInput::ToolType type);

// "Converts" a C++ StrokeInput object into a jobject of type StrokeInput by
// overwriting the fields of `j_input_out` using the values of `input_in`.
// Return error JVM exception is pending or if overwrite operation failed.
// Accepts InputToolType.class from the Java/Kotlin side as a convenience to
// avoid a reflection-based FindClass lookup or other ways to access the jclass
// for InputToolType.
void UpdateJObjectInput(JNIEnv* env, const StrokeInput& input_in,
                        jobject j_input_out, jclass inputtooltype_class);

// "Converts" a kotlin StrokeInput to a C++ StrokeInput object.
StrokeInput JObjectToStrokeInput(JNIEnv* env, jobject j_input);

}  // namespace ink::jni

#endif  // INK_STROKES_INTERNAL_JNI_STROKE_INPUT_JNI_HELPER_H_
