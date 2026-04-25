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

#include <jni.h>

#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/jni/internal/status_native.h"

using ::ink::jni::JStringToStdString;
using ::ink::jni::ThrowExceptionFromStatusCallback;

namespace {}

extern "C" {

JNI_METHOD(nativeloader, StatusNative, jint, statusCodeOk)
(JNIEnv* env, jobject object) { return StatusNative_statusCodeOk(); }

JNI_METHOD(nativeloader, StatusNative, jint, statusCodeFailedPrecondition)
(JNIEnv* env, jobject object) {
  return StatusNative_statusCodeFailedPrecondition();
}

JNI_METHOD(nativeloader, StatusNative, jint, statusCodeInvalidArgument)
(JNIEnv* env, jobject object) {
  return StatusNative_statusCodeInvalidArgument();
}

JNI_METHOD(nativeloader, StatusNative, jint, statusCodeNotFound)
(JNIEnv* env, jobject object) { return StatusNative_statusCodeNotFound(); }

JNI_METHOD(nativeloader, StatusNative, jint, statusCodeOutOfRange)
(JNIEnv* env, jobject object) { return StatusNative_statusCodeOutOfRange(); }

JNI_METHOD(nativeloader, StatusNative, jint, statusCodeUnimplemented)
(JNIEnv* env, jobject object) { return StatusNative_statusCodeUnimplemented(); }

JNI_METHOD(nativeloader, StatusNative, void,
           throwExceptionFromOkStatusForTesting)
(JNIEnv* env, jobject object) {
  StatusNative_throwExceptionFromOkStatusForTesting(
      env, &ThrowExceptionFromStatusCallback);
}

JNI_METHOD(nativeloader, StatusNative, void,
           throwExceptionFromFailedPreconditionForTesting)
(JNIEnv* env, jobject object, jstring message) {
  StatusNative_throwExceptionFromFailedPreconditionForTesting(
      env, &ThrowExceptionFromStatusCallback,
      JStringToStdString(env, message).c_str());
}

JNI_METHOD(nativeloader, StatusNative, void,
           throwExceptionFromInvalidArgumentForTesting)
(JNIEnv* env, jobject object, jstring message) {
  StatusNative_throwExceptionFromInvalidArgumentForTesting(
      env, &ThrowExceptionFromStatusCallback,
      JStringToStdString(env, message).c_str());
}

JNI_METHOD(nativeloader, StatusNative, void,
           throwExceptionFromNotFoundForTesting)
(JNIEnv* env, jobject object, jstring message) {
  StatusNative_throwExceptionFromNotFoundForTesting(
      env, &ThrowExceptionFromStatusCallback,
      JStringToStdString(env, message).c_str());
}

JNI_METHOD(nativeloader, StatusNative, void,
           throwExceptionFromOutOfRangeForTesting)
(JNIEnv* env, jobject object, jstring message) {
  StatusNative_throwExceptionFromOutOfRangeForTesting(
      env, &ThrowExceptionFromStatusCallback,
      JStringToStdString(env, message).c_str());
}

JNI_METHOD(nativeloader, StatusNative, void,
           throwExceptionFromUnimplementedForTesting)
(JNIEnv* env, jobject object, jstring message) {
  StatusNative_throwExceptionFromUnimplementedForTesting(
      env, &ThrowExceptionFromStatusCallback,
      JStringToStdString(env, message).c_str());
}

JNI_METHOD(nativeloader, StatusNative, void,
           throwExceptionFromUnknownStatusCodeForTesting)
(JNIEnv* env, jobject object, jint status_code, jstring message) {
  StatusNative_throwExceptionFromUnknownStatusCodeForTesting(
      env, &ThrowExceptionFromStatusCallback, status_code,
      JStringToStdString(env, message).c_str());
}

}  // extern "C"
