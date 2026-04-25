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

#include "ink/geometry/internal/jni/affine_transform_native.h"

#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/quad.h"

using ::ink::AffineTransform;
using ::ink::Angle;
using ::ink::Quad;

extern "C" {

AffineTransformNative_Parallelogram AffineTransformNative_apply(
    float a, float b, float c, float d, float e, float f, float quad_center_x,
    float quad_center_y, float quad_width, float quad_height,
    float quad_rotation_degrees, float quad_shear_factor) {
  Quad transformed =
      AffineTransform(a, b, c, d, e, f)
          .Apply(Quad::FromCenterDimensionsRotationAndSkew(
              {.x = quad_center_x, .y = quad_center_y}, quad_width, quad_height,
              Angle::Degrees(quad_rotation_degrees), quad_shear_factor));
  return {{transformed.Center().x, transformed.Center().y},
          transformed.Width(),
          transformed.Height(),
          transformed.Rotation().ValueInDegrees(),
          transformed.Skew()};
}

}  // extern "C"
