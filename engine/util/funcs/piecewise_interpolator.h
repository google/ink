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

#ifndef INK_ENGINE_UTIL_FUNCS_PIECEWISE_INTERPOLATOR_H_
#define INK_ENGINE_UTIL_FUNCS_PIECEWISE_INTERPOLATOR_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"

namespace ink {

// Utility class for doing piecewise interpolation based upon a series of points
// defining the line segments.
class PiecewiseInterpolator {
 public:
  // Construct a piecewise linear interpolator with the given points. Points
  // list must not be empty. Search is linear and as a precaution this will
  // reject point lists of size >= 20.
  explicit PiecewiseInterpolator(const std::vector<glm::vec2> &points);

  // Get the interpolated value of the function at x. If the input x is before
  // or after the added points, the first or last y value is returned.
  float GetValue(float x) const;

 private:
  std::vector<glm::vec2> points_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_FUNCS_PIECEWISE_INTERPOLATOR_H_
