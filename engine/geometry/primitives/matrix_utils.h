/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_GEOMETRY_PRIMITIVES_MATRIX_UTILS_H_
#define INK_ENGINE_GEOMETRY_PRIMITIVES_MATRIX_UTILS_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/geometry_portable_proto.pb.h"

namespace ink {
namespace matrix_utils {

// Returns true if there exists an inverse matrix n, such that
// m * n = n * m = the identity matrix.
bool IsInvertible(const glm::mat4& m);

// Returns true if the matrix is of the form:
// ⎡a   b   0   c⎤
// ⎢d   e   0   f⎥
// ⎢0   0   1   0⎥
// ⎣0   0   0   1⎦
// Note that this does not check for invertibility. In most cases, you'll want
// to use IsValidElementTransform().
bool IsAffineTransform(const glm::mat4& m);

// Returns true if matrix m is an invertible affine transform such that
// m = T * R * S, where T is a translation matrix in the xy-plane, R is a
// rotation matrix about the positive z-axis, and S is a scale matrix in
// the x- and y-directions.
bool IsValidElementTransform(const glm::mat4& m);

// Convenience function to construct an affine transform matrix of the form:
// ⎡a   b   0   c⎤
// ⎢d   e   0   f⎥
// ⎢0   0   1   0⎥
// ⎣0   0   0   1⎦
inline glm::mat4 AffineTransformMatrix(float a, float b, float c, float d,
                                       float e, float f) {
  return glm::mat4{{a, d, 0, 0}, {b, e, 0, 0}, {0, 0, 1, 0}, {c, f, 0, 1}};
}

// Extracts the translation component of a valid element transform.
inline glm::vec2 GetTranslationComponent(const glm::mat4& m) {
  return {m[3][0], m[3][1]};
}

// Extracts the rotation component of a valid element transform. The return
// value will lie in the interval (-π, π].
// Note: The transform rotate(θ) * scale(x, y) is equivalent to the transform
// rotate(θ ± π/2) * scale(-x, y). This function assumes that x-scale component
// of the transform is positive.
inline float GetRotationComponent(const glm::mat4& m) {
  return std::atan2(m[0][1], m[0][0]);
}

// Extracts the x- and y-scale components of a valid element transform.
// Note: The transforms rotate(θ) * scale(x, y) is equivalent to the transform
// rotate(θ ± π/2) * scale(-x, y). This function assumes that x-direction scale
// component of the transform is positive.
inline glm::vec2 GetScaleComponent(const glm::mat4& m) {
  return {glm::length(glm::vec2{m[0][0], m[0][1]}),
          std::copysign(glm::length(glm::vec2{m[1][0], m[1][1]}),
                        m[0][0] * m[1][1])};
}

// Convenience function to find the average of the absolute values of the x- and
// y-scale components.
inline float GetAverageAbsScale(const glm::mat4& m) {
  glm::vec2 scale = GetScaleComponent(m);
  return .5f * (std::abs(scale.x) + std::abs(scale.y));
}

// Returns a transformation matrix that scales by the given factor, using the
// given point as the scaling center.
// For scaling factor k and point (x, y), the matrix will be of the form:
// ⎡k   0   0   x−kx⎤
// ⎢0   k   0   y−ky⎥
// ⎢0   0   1     0 ⎥
// ⎣0   0   0     1 ⎦
glm::mat4 ScaleAboutPoint(float scaling_factor, glm::vec2 point);

// Returns a transformation matrix that rotates counterclockwise by the given
// angle, using the given point as the center of rotation.
// For angle θ and point (x, y), the matrix will be of the form:
// ⎡cosθ   -sinθ   0   x−x⋅cosθ+y⋅sinθ⎤
// ⎢sinθ    cosθ   0   y−x⋅sinθ−y⋅cosθ⎥
// ⎢ 0       0     1          0       ⎥
// ⎣ 0       0     0          1       ⎦
glm::mat4 RotateAboutPoint(float radians, glm::vec2 point);

}  // namespace matrix_utils

// Template specializations for proto conversion.
namespace util {
template <>
S_WARN_UNUSED_RESULT Status ReadFromProto(const proto::AffineTransform& proto,
                                          glm::mat4* mat);
template <>
void WriteToProto(proto::AffineTransform* proto, const glm::mat4& mat);

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_PRIMITIVES_MATRIX_UTILS_H_
