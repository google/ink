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

#include "ink/brush/internal/jni/input_tool_type_native.h"
#include "ink/jni/internal/jni_defines.h"

extern "C" {

JNI_METHOD(brush, InputToolTypeNative, jint, calculateMinimumRequiredVersion)
(JNIEnv* env, jobject thiz, jint tool_type_int) {
  return InputToolTypeNative_calculateMinimumRequiredVersion(tool_type_int);
}

}  // extern "C"
