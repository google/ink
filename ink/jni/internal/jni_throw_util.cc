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

namespace ink::jni {

namespace {

const char* ExceptionClassForStatusCode(absl::StatusCode code) {
  // These should all be subclasses of RuntimeException.
  switch (code) {
    case absl::StatusCode::kFailedPrecondition:
      return "java/lang/IllegalStateException";
    case absl::StatusCode::kInvalidArgument:
      return "java/lang/IllegalArgumentException";
    case absl::StatusCode::kNotFound:
      return "java/util/NoSuchElementException";
    case absl::StatusCode::kOutOfRange:
      return "java/lang/IndexOutOfBoundsException";
    case absl::StatusCode::kUnimplemented:
      return "java/lang/UnsupportedOperationException";
    default:
      return "java/lang/RuntimeException";
  }
}

void ThrowException(JNIEnv* env, const char* java_exception_path,
                    const std::string& message) {
  jclass exception_class = env->FindClass(java_exception_path);
  ABSL_CHECK(exception_class);
  env->ThrowNew(exception_class, message.c_str());
}

}  // namespace

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

}  // namespace ink::jni
