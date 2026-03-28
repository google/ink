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

#include "ink/geometry/internal/jni/parallelogram_native.h"

#include <array>
#include <utility>

#include "ink/geometry/angle.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/point.h"
#include "ink/geometry/quad.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/vec.h"

using ::ink::Angle;
using ::ink::Envelope;
using ::ink::Point;
using ::ink::Quad;
using ::ink::Rect;
using ::ink::Vec;

extern "C" {

ParallelogramNative_Box ParallelogramNative_computeBoundingBox(
    float center_x, float center_y, float width, float height,
    float rotation_degrees, float skew) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  Rect rect = Envelope(quad).AsRect().value();
  return {rect.XMin(), rect.YMin(), rect.XMax(), rect.YMax()};
}

ParallelogramNative_SemiAxes ParallelogramNative_computeSemiAxes(
    float center_x, float center_y, float width, float height,
    float rotation_degrees, float skew) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  std::pair<Vec, Vec> axes = quad.SemiAxes();
  return {{axes.first.x, axes.first.y}, {axes.second.x, axes.second.y}};
}

ParallelogramNative_Corners ParallelogramNative_computeCorners(
    float center_x, float center_y, float width, float height,
    float rotation_degrees, float skew) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  std::array<Point, 4> corners = quad.Corners();
  return {{corners[0].x, corners[0].y},
          {corners[1].x, corners[1].y},
          {corners[2].x, corners[2].y},
          {corners[3].x, corners[3].y}};
}

bool ParallelogramNative_contains(float center_x, float center_y, float width,
                                  float height, float rotation_degrees,
                                  float skew, float point_x, float point_y) {
  Quad quad = Quad::FromCenterDimensionsRotationAndSkew(
      {center_x, center_y}, width, height, Angle::Degrees(rotation_degrees),
      skew);
  return quad.Contains({point_x, point_y});
}

}  // extern "C"
