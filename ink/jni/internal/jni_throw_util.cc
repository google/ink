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

#include "ink/jni/internal/jni_throw_util.h"

#include <jni.h>

#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/jni/internal/jni_string_util.h"

namespace ink {
namespace jni {

namespace {

const std::pair<absl::StatusCode, const char*> kStatusCodeClasses[] = {
    {absl::StatusCode::kFailedPrecondition, "java/lang/IllegalStateException"},
    {absl::StatusCode::kInvalidArgument, "java/lang/IllegalArgumentException"},
    {absl::StatusCode::kNotFound, "java/util/NoSuchElementException"},
    {absl::StatusCode::kOutOfRange, "java/lang/IndexOutOfBoundsException"},
    {absl::StatusCode::kUnimplemented,
     "java/lang/UnsupportedOperationException"},
};

const char* ExceptionClassForStatusCode(absl::StatusCode code) {
  for (auto [status_code, class_name] : kStatusCodeClasses) {
    if (status_code == code) return class_name;
  }
  return "java/lang/IllegalStateException";
}

absl::StatusCode StatusCodeForThrowable(JNIEnv* env, jthrowable exception) {
  ABSL_CHECK(exception);
  for (auto [status_code, class_name] : kStatusCodeClasses) {
    jclass clazz = env->FindClass(class_name);
    ABSL_CHECK(clazz);
    if (env->IsInstanceOf(exception, clazz)) {
      return status_code;
    }
  }
  return absl::StatusCode::kUnknown;
}

// Returns the message string for a JVM exception. Returns null if the exception
// is null, or if the exception has no message.
jstring GetExceptionMessage(JNIEnv* env, jthrowable exception) {
  if (exception == nullptr) return nullptr;
  jmethodID method = env->GetMethodID(env->GetObjectClass(exception),
                                      "getMessage", "()Ljava/lang/String;");
  ABSL_CHECK(method);
  return static_cast<jstring>(env->CallObjectMethod(exception, method));
}

}  // namespace

absl::Status CatchExceptionAsStatus(JNIEnv* env) {
  jthrowable exception = env->ExceptionOccurred();
  if (exception == nullptr) return absl::OkStatus();
  env->ExceptionClear();
  return absl::Status(
      StatusCodeForThrowable(env, exception),
      JStringView(env, GetExceptionMessage(env, exception)).string_view());
}

bool CheckOkOrThrow(JNIEnv* env, const absl::Status& status) {
  if (status.ok()) {
    return true;
  }
  ThrowExceptionFromStatus(env, status);
  return false;
}

void ThrowExceptionFromStatus(JNIEnv* env, const absl::Status& status) {
  ABSL_CHECK(!status.ok());
  ThrowException(env, ExceptionClassForStatusCode(status.code()),
                 status.ToString());
}

void ThrowException(JNIEnv* env, const char* java_exception_path,
                    const std::string& message) {
  jclass exception_class = env->FindClass(java_exception_path);
  ABSL_CHECK(exception_class);
  env->ThrowNew(exception_class, message.c_str());
}

}  // namespace jni
}  // namespace ink
