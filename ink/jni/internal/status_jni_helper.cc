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

#include "ink/jni/internal/status_jni_helper.h"

#include <jni.h>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/jni/internal/jni_jvm_interface.h"

namespace ink::jni {

void ThrowExceptionFromStatus(JNIEnv* env, const absl::Status& status) {
  // If an exception was already raised (e.g. from an earlier callback),
  // propagate that as-is.
  if (env->ExceptionCheck()) {
    return;
  }
  env->CallStaticVoidMethod(
      ClassNativeExceptionHandling(env),
      MethodNativeExceptionHandlingThrowForNonOkStatus(env), status.code(),
      env->NewStringUTF(
          status.ToString(absl::StatusToStringMode::kWithEverything).c_str()));
}

void ThrowExceptionFromStatusCallback(void* jni_env, const void* status) {
  ABSL_CHECK(status != nullptr);
  ThrowExceptionFromStatus(static_cast<JNIEnv*>(jni_env),
                           *static_cast<const absl::Status*>(status));
}

}  // namespace ink::jni
