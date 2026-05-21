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

#ifndef INK_STROKES_INTERNAL_JNI_STROKE_INPUT_JNI_HELPER_H_
#define INK_STROKES_INTERNAL_JNI_STROKE_INPUT_JNI_HELPER_H_

#include <jni.h>

#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/internal/jni/in_progress_stroke_native.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native.h"

namespace ink::jni {

// Calls back into the JVM to populate an existing Kotlin StrokeInput object
// with the provided StrokeInput. The caller must check if an exception was
// thrown by this call, e.g. with env->ExceptionCheck(). If an exception was
// thrown, the caller must bail out instead of continuing execution.
void UpdateJStrokeInputOrThrow(JNIEnv* env, const StrokeInput& input_in,
                               jobject j_input_out);
void UpdateJStrokeInputOrThrow(JNIEnv* env,
                               const StrokeInputBatchNative_Input& input_in,
                               jobject j_input_out);
void UpdateJStrokeInputOrThrow(JNIEnv* env,
                               const InProgressStrokeNative_Input& input_in,
                               jobject j_input_out);

}  // namespace ink::jni

#endif  // INK_STROKES_INTERNAL_JNI_STROKE_INPUT_JNI_HELPER_H_
