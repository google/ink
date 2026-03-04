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

#include "ink/jni/internal/jni_defines.h"

extern "C" {

// C-compatible library header needs to be included in extern "C" block.
#include "ink/geometry/internal/jni/angle_native.h"

JNI_METHOD(geometry, AngleNative, jfloat, normalizedRadians)
(JNIEnv* env, jobject object, jfloat radians) {
  return AngleNative_normalizedRadians(radians);
}

JNI_METHOD(geometry, AngleNative, jfloat, normalizedAboutZeroRadians)
(JNIEnv* env, jobject object, jfloat radians) {
  return AngleNative_normalizedAboutZeroRadians(radians);
}

JNI_METHOD(geometry, AngleNative, jfloat, normalizedDegrees)
(JNIEnv* env, jobject object, jfloat degrees) {
  return AngleNative_normalizedDegrees(degrees);
}

JNI_METHOD(geometry, AngleNative, jfloat, normalizedAboutZeroDegrees)
(JNIEnv* env, jobject object, jfloat degrees) {
  return AngleNative_normalizedAboutZeroDegrees(degrees);
}

}  // extern "C"
