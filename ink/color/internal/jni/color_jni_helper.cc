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

#include "ink/color/internal/jni/color_jni_helper.h"

#include <jni.h>

#include <cstdint>

#include "ink/jni/internal/jni_jvm_interface.h"

namespace ink::jni {

int64_t ComposeColorLongFromComponentsCallback(
    void* jni_env, int color_space_id, float red_gamma_corrected,
    float green_gamma_corrected, float blue_gamma_corrected, float alpha) {
  JNIEnv* env = static_cast<JNIEnv*>(jni_env);
  return env->CallStaticLongMethod(
      ClassColorCallbacks(env),
      MethodColorCallbacksComposeColorLongFromComponents(env), color_space_id,
      red_gamma_corrected, green_gamma_corrected, blue_gamma_corrected, alpha);
}

}  // namespace ink::jni
