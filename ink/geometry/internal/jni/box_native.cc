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

#include "ink/geometry/internal/jni/box_native.h"

#include "ink/geometry/point.h"
#include "ink/geometry/rect.h"

using ::ink::Point;
using ::ink::Rect;

extern "C" {

BoxNative_Vec BoxNative_createCenter(float rect_x_min, float rect_y_min,
                                     float rect_x_max, float rect_y_max) {
  Rect rect =
      Rect::FromTwoPoints({rect_x_min, rect_y_min}, {rect_x_max, rect_y_max});
  Point center = rect.Center();
  return {center.x, center.y};
}

bool BoxNative_containsPoint(float rect_x_min, float rect_y_min,
                             float rect_x_max, float rect_y_max, float point_x,
                             float point_y) {
  Rect rect =
      Rect::FromTwoPoints({rect_x_min, rect_y_min}, {rect_x_max, rect_y_max});
  return rect.Contains(Point{point_x, point_y});
}

bool BoxNative_containsBox(float rect_x_min, float rect_y_min, float rect_x_max,
                           float rect_y_max, float other_x_min,
                           float other_y_min, float other_x_max,
                           float other_y_max) {
  Rect rect =
      Rect::FromTwoPoints({rect_x_min, rect_y_min}, {rect_x_max, rect_y_max});
  Rect other_rect = Rect::FromTwoPoints({other_x_min, other_y_min},
                                        {other_x_max, other_y_max});
  return rect.Contains(other_rect);
}

}  // extern "C"
