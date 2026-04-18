// Copyright 2024-2026 Google LLC
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

#include "ink/jni/internal/status_jni_helper.h"

#include <jni.h>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "ink/jni/internal/jni_jvm_interface.h"

namespace ink::jni {

namespace {

void ThrowExceptionFromStatus(JNIEnv* env, absl::StatusCode status_code,
                              absl::string_view status_string) {
  env->CallStaticVoidMethod(
      ClassNativeExceptionHandling(env),
      MethodNativeExceptionHandlingThrowForNonOkStatus(env), status_code,
      env->NewStringUTF(status_string.data()));
}

}  // namespace

void ThrowExceptionFromStatus(JNIEnv* env, const absl::Status& status) {
  ThrowExceptionFromStatus(env, status.code(), status.ToString());
}

void ThrowExceptionFromStatusCallback(void* jni_env, int status_code,
                                      const char* status_string) {
  ThrowExceptionFromStatus(static_cast<JNIEnv*>(jni_env),
                           static_cast<absl::StatusCode>(status_code),
                           status_string == nullptr ? "" : status_string);
}

}  // namespace ink::jni
