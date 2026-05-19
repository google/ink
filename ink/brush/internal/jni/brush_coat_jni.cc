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

#include <cstdint>

#include "absl/log/absl_check.h"
#include "ink/brush/internal/jni/brush_coat_native.h"
#include "ink/jni/internal/jni_defines.h"

extern "C" {

// Construct a native BrushCoat and return a pointer to it as a long.
JNI_METHOD(brush, BrushCoatNative, jlong, create)
(JNIEnv* env, jobject thiz, jlong tip_native_pointer,
 jlongArray paint_preferences_native_pointers_array) {
  ABSL_CHECK(paint_preferences_native_pointers_array != nullptr);
  const jsize paint_preferences_count =
      env->GetArrayLength(paint_preferences_native_pointers_array);
  jlong* paint_preferences_native_pointers = env->GetLongArrayElements(
      paint_preferences_native_pointers_array, nullptr);
  // Both `jlong` and `int64_t` are required to be 64-bit integers which JNI and
  // Kotlin-cinterop respectively both map to Kotlin `Long`. However, on MacOS
  // they represent two distinct (though equivalent) types, `jlong` is `long`
  // but `int64_t` is `long long`.
  static_assert(sizeof(jlong) == sizeof(int64_t));
  jlong result = BrushCoatNative_create(
      tip_native_pointer,
      reinterpret_cast<int64_t*>(paint_preferences_native_pointers),
      paint_preferences_count);
  env->ReleaseLongArrayElements(paint_preferences_native_pointers_array,
                                paint_preferences_native_pointers, JNI_ABORT);
  return result;
}

JNI_METHOD(brush, BrushCoatNative, jboolean, isCompatibleWithMeshFormat)
(JNIEnv* env, jobject obj, jlong native_pointer,
 jlong mesh_format_native_pointer) {
  return BrushCoatNative_isCompatibleWithMeshFormat(native_pointer,
                                                    mesh_format_native_pointer);
}

JNI_METHOD(brush, BrushCoatNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  BrushCoatNative_free(native_pointer);
}

JNI_METHOD(brush, BrushCoatNative, jlong, newCopyOfBrushTip)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushCoatNative_newCopyOfBrushTip(native_pointer);
}

JNI_METHOD(brush, BrushCoatNative, jint, getBrushPaintPreferencesCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushCoatNative_getBrushPaintPreferencesCount(native_pointer);
}

JNI_METHOD(brush, BrushCoatNative, jlong, newCopyOfBrushPaintPreference)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushCoatNative_newCopyOfBrushPaintPreference(native_pointer, index);
}

}  // extern "C"
