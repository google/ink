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

#include <jni.h>

#include "ink/brush/internal/jni/brush_native.h"
#include "ink/color/internal/jni/color_jni_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"

using ::ink::jni::ComposeColorLongFromComponentsCallback;
using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

JNI_METHOD(brush, BrushNative, jlong, create)
(JNIEnv* env, jobject object, jlong family_native_pointer, jfloat color_red,
 jfloat color_green, jfloat color_blue, jfloat color_alpha, jint color_space_id,
 jfloat size, jfloat epsilon) {
  return BrushNative_create(env, family_native_pointer, color_red, color_green,
                            color_blue, color_alpha, color_space_id, size,
                            epsilon, &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(brush, BrushNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  BrushNative_free(native_pointer);
}

JNI_METHOD(brush, BrushNative, jlong, computeComposeColorLong)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushNative_computeComposeColorLong(
      env, native_pointer, &ComposeColorLongFromComponentsCallback);
}

JNI_METHOD(brush, BrushNative, jfloat, getSize)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushNative_getSize(native_pointer);
}

JNI_METHOD(brush, BrushNative, jfloat, getEpsilon)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushNative_getEpsilon(native_pointer);
}

JNI_METHOD(brush, BrushNative, jlong, newCopyOfBrushFamily)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushNative_newCopyOfBrushFamily(native_pointer);
}

}  // extern "C"
