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

#ifndef INK_GEOMETRY_INTERNAL_JNI_PARALLELOGRAM_NATIVE_H_
#define INK_GEOMETRY_INTERNAL_JNI_PARALLELOGRAM_NATIVE_H_

#include <stdbool.h>

// C-compatible library header for Kotlin-native bindings.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x;
  float y;
} ParallelogramNative_Vec;

typedef struct {
  ParallelogramNative_Vec first;
  ParallelogramNative_Vec second;
} ParallelogramNative_SemiAxes;

typedef struct {
  float xmin;
  float ymin;
  float xmax;
  float ymax;
} ParallelogramNative_Box;

typedef struct {
  ParallelogramNative_Vec corner1;
  ParallelogramNative_Vec corner2;
  ParallelogramNative_Vec corner3;
  ParallelogramNative_Vec corner4;
} ParallelogramNative_Corners;

ParallelogramNative_Box ParallelogramNative_computeBoundingBox(
    float center_x, float center_y, float width, float height,
    float rotation_degrees, float skew);

ParallelogramNative_SemiAxes ParallelogramNative_computeSemiAxes(
    float center_x, float center_y, float width, float height,
    float rotation_degrees, float skew);

ParallelogramNative_Corners ParallelogramNative_computeCorners(
    float center_x, float center_y, float width, float height,
    float rotation_degrees, float skew);

bool ParallelogramNative_contains(float center_x, float center_y, float width,
                                  float height, float rotation_degrees,
                                  float skew, float point_x, float point_y);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_GEOMETRY_INTERNAL_JNI_PARALLELOGRAM_NATIVE_H_
