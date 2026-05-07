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

#include <jni.h>

#include <cstdint>
#include <string>

#include "ink/brush/internal/jni/brush_family_native.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/status_jni_helper.h"

using ::ink::jni::JStringToStdString;
using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

JNI_METHOD(brush, BrushFamilyNative, jlong,
           create)(JNIEnv* env, jobject object,
                   jlongArray coat_native_pointer_array,
                   jlong input_model_pointer, jstring client_brush_family_id,
                   jstring developer_comment) {
  std::string cbf_id = JStringToStdString(env, client_brush_family_id);
  std::string dev_comment = JStringToStdString(env, developer_comment);
  const jsize num_coats = env->GetArrayLength(coat_native_pointer_array);
  jlong* coat_pointers =
      env->GetLongArrayElements(coat_native_pointer_array, nullptr);

  // Both `jlong` and `int64_t` are required to be 64-bit integers which JNI and
  // Kotlin-cinterop respectively both map to Kotlin `Long`. However, on MacOS
  // they represent two distinct (though equivalent) types, `jlong` is `long`
  // but `int64_t` is `long long`.
  static_assert(sizeof(jlong) == sizeof(int64_t));
  jlong result = BrushFamilyNative_create(
      env, reinterpret_cast<int64_t*>(coat_pointers), num_coats,
      input_model_pointer, cbf_id.c_str(), dev_comment.c_str(),
      &ThrowExceptionFromStatusCallback);
  env->ReleaseLongArrayElements(coat_native_pointer_array, coat_pointers,
                                JNI_ABORT);
  return result;
}

JNI_METHOD(brush, BrushFamilyNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  BrushFamilyNative_free(native_pointer);
}

JNI_METHOD(brush, BrushFamilyNative, jstring, getClientBrushFamilyId)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return env->NewStringUTF(
      BrushFamilyNative_getClientBrushFamilyId(native_pointer));
}

JNI_METHOD(brush, BrushFamilyNative, jstring, getDeveloperComment)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return env->NewStringUTF(
      BrushFamilyNative_getDeveloperComment(native_pointer));
}

JNI_METHOD(brush, BrushFamilyNative, jlong, getBrushCoatCount)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushFamilyNative_getBrushCoatCount(native_pointer);
}

JNI_METHOD(brush, BrushFamilyNative, jlong, newCopyOfBrushCoat)
(JNIEnv* env, jobject object, jlong native_pointer, jint index) {
  return BrushFamilyNative_newCopyOfBrushCoat(native_pointer, index);
}

JNI_METHOD(brush, BrushFamilyNative, jint, getInputModelType)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushFamilyNative_getInputModelType(native_pointer);
}

JNI_METHOD(brush, BrushFamilyNative, jlong, newCopyOfInputModel)
(JNIEnv* env, jobject object, jlong native_pointer, jint index) {
  return BrushFamilyNative_newCopyOfInputModel(native_pointer);
}

JNI_METHOD(brush, BrushFamilyNative, jint, calculateMinimumRequiredVersion)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushFamilyNative_calculateMinimumRequiredVersion(native_pointer);
}

JNI_METHOD(brush, BrushFamilyNative, jboolean, hasFallbacks)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return BrushFamilyNative_hasFallbacks(native_pointer);
}

JNI_METHOD(brush, InputModelNative, jlong, createNoParametersModel)
(JNIEnv* env, jobject object, jint type) {
  return InputModelNative_createNoParametersModel(type);
}

JNI_METHOD(brush, InputModelNative, jlong, createSlidingWindowModel)
(JNIEnv* env, jobject object, jlong window_size_millis,
 jint upsampling_frequency_hz) {
  return InputModelNative_createSlidingWindowModel(window_size_millis,
                                                   upsampling_frequency_hz);
}

JNI_METHOD(brush, InputModelNative, jlong,
           createSlidingWindowModelWithDefaultParameters)
(JNIEnv* env, jobject object) {
  return InputModelNative_createSlidingWindowModelWithDefaultParameters();
}

JNI_METHOD(brush, InputModelNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  InputModelNative_free(native_pointer);
}

JNI_METHOD(brush, InputModelNative, jlong, getSlidingWindowDurationMillis)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return InputModelNative_getSlidingWindowDurationMillis(native_pointer);
}

JNI_METHOD(brush, InputModelNative, jint, getSlidingUpsamplingFrequencyHz)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return InputModelNative_getSlidingUpsamplingFrequencyHz(native_pointer);
}

}  // extern "C"
