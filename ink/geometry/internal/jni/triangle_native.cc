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

#include "ink/geometry/internal/jni/triangle_native.h"

#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"

using ::ink::Point;
using ::ink::Triangle;

extern "C" {

bool TriangleNative_contains(float triangle_p0_x, float triangle_p0_y,
                             float triangle_p1_x, float triangle_p1_y,
                             float triangle_p2_x, float triangle_p2_y,
                             float point_x, float point_y) {
  Point point = Point{point_x, point_y};
  Triangle triangle{{triangle_p0_x, triangle_p0_y},
                    {triangle_p1_x, triangle_p1_y},
                    {triangle_p2_x, triangle_p2_y}};
  return triangle.Contains(point);
}

}  // extern "C"
