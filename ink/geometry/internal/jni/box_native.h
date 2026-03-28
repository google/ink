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

#ifndef INK_GEOMETRY_INTERNAL_JNI_BOX_NATIVE_H_
#define INK_GEOMETRY_INTERNAL_JNI_BOX_NATIVE_H_

#include <stdbool.h>

// C-compatible library header for Kotlin-native bindings.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x;
  float y;
} BoxNative_Vec;

BoxNative_Vec BoxNative_createCenter(float rect_x_min, float rect_y_min,
                                     float rect_x_max, float rect_y_max);

bool BoxNative_containsPoint(float rect_x_min, float rect_y_min,
                             float rect_x_max, float rect_y_max, float point_x,
                             float point_y);

bool BoxNative_containsBox(float rect_x_min, float rect_y_min, float rect_x_max,
                           float rect_y_max, float other_x_min,
                           float other_y_min, float other_x_max,
                           float other_y_max);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_GEOMETRY_INTERNAL_JNI_BOX_NATIVE_H_
