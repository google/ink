// Copyright 2018 Google LLC
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

#include "ink/engine/geometry/primitives/matrix_utils.h"

#include <cmath>

#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {
namespace matrix_utils {
namespace {
// We allow for some wiggle room in the matrices, to account for precision loss.
constexpr float kEqualTol = 0.0001;
bool EqualWithinTol(float lhs, float rhs) {
  return std::abs(lhs - rhs) < kEqualTol;
}
}  // namespace

bool IsInvertible(const glm::mat4& m) {
  const float det = glm::determinant(m);
  return det != 0 && !std::isnan(det);
}

bool IsAffineTransform(const glm::mat4& m) {
  return
      // First column
      EqualWithinTol(m[0][2], 0) && EqualWithinTol(m[0][3], 0) &&
      // Second column
      EqualWithinTol(m[1][2], 0) && EqualWithinTol(m[1][3], 0) &&
      // Third column
      EqualWithinTol(m[2][0], 0) && EqualWithinTol(m[2][1], 0) &&
      EqualWithinTol(m[2][2], 1) && EqualWithinTol(m[2][3], 0) &&
      // Fourth column
      EqualWithinTol(m[3][2], 0) && EqualWithinTol(m[3][3], 1);
}

bool IsValidElementTransform(const glm::mat4& m) {
  if (!IsAffineTransform(m) || !IsInvertible(m)) return false;

  // Check that the matrix preserves angles.
  return EqualWithinTol(m[0][0] * m[1][0] + m[0][1] * m[1][1], 0);
}

glm::mat4 ScaleAboutPoint(float scaling_factor, glm::vec2 point) {
  if (scaling_factor == 1) return glm::mat4{1};

  return glm::translate(glm::mat4{1}, glm::vec3(point, 0)) *
         glm::scale(glm::mat4{1},
                    glm::vec3(scaling_factor, scaling_factor, 1)) *
         glm::translate(glm::mat4{1}, glm::vec3(-point, 0));
}

glm::mat4 RotateAboutPoint(float radians, glm::vec2 point) {
  if (radians == 0) return glm::mat4{1};

  float cosine = std::cos(radians);
  float sine = std::sin(radians);
  return {{cosine, sine, 0, 0},
          {-sine, cosine, 0, 0},
          {0, 0, 1, 0},
          {-point.x * cosine + point.x + point.y * sine,
           -point.x * sine - point.y * cosine + point.y, 0, 1}};
}

}  // namespace matrix_utils

namespace util {
using proto::AffineTransform;

template <>
Status ReadFromProto(const AffineTransform& proto, glm::mat4* mat) {
  ASSERT(mat);

  float cos_angle = std::cos(proto.rotation_radians());
  float sin_angle = std::sin(proto.rotation_radians());

  *mat = glm::mat4{1};
  (*mat)[0][0] = proto.scale_x() * cos_angle;
  (*mat)[0][1] = proto.scale_x() * sin_angle;
  (*mat)[1][0] = -proto.scale_y() * sin_angle;
  (*mat)[1][1] = proto.scale_y() * cos_angle;
  (*mat)[3][0] = proto.tx();
  (*mat)[3][1] = proto.ty();
  if (!matrix_utils::IsInvertible(*mat)) {
    // clear it
    *mat = glm::mat4{1};
    return status::InvalidArgument(
        "Rejecting non-invertible transform matrix.");
  }
  return OkStatus();
}

template <>
void WriteToProto(AffineTransform* proto, const glm::mat4& mat) {
  ASSERT(proto);
  if (!matrix_utils::IsValidElementTransform(mat)) {
    LOG(ERROR) << "Matrix is not a valid element transform.";
  }

  glm::vec2 translation = matrix_utils::GetTranslationComponent(mat);
  float rotation = matrix_utils::GetRotationComponent(mat);
  glm::vec2 scale = matrix_utils::GetScaleComponent(mat);

  proto->Clear();
  proto->set_scale_x(scale.x);
  proto->set_scale_y(scale.y);
  proto->set_tx(translation.x);
  proto->set_ty(translation.y);
  proto->set_rotation_radians(rotation);
}

}  // namespace util
}  // namespace ink
