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

#ifndef INK_ENGINE_UTIL_ANIMATION_INTERPOLATOR_H_
#define INK_ENGINE_UTIL_ANIMATION_INTERPOLATOR_H_

#include <memory>

#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

// An Interpolator calculates the intermediate value between two end points
// given progress from 0 to 1.
// Interpolator implementations must have the property that:
// Interpolate(a, b, 0) == a
// Interpolate(a, b, 1) == b
//
// Note that progress could be <0 or >1 depending on the Animation curve: the
// expected behavior is extrapolation in those cases.
template <typename T>
class Interpolator {
 public:
  virtual ~Interpolator() {}
  virtual T Interpolate(const T& from, const T& to, float progress) {
    return util::Lerpnc(from, to, progress);
  }
};

template <typename T>
std::unique_ptr<Interpolator<T>> DefaultInterpolator() {
  return std::unique_ptr<Interpolator<T>>(new Interpolator<T>());
}

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_INTERPOLATOR_H_
