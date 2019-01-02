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

#ifndef INK_ENGINE_UTIL_ANIMATION_ANIMATED_VALUE_H_
#define INK_ENGINE_UTIL_ANIMATION_ANIMATED_VALUE_H_

#include <functional>
#include <memory>

#include "ink/engine/util/animation/animated_fn.h"
#include "ink/engine/util/animation/animation_controller.h"

namespace ink {

// An AnimatedValue<T> is something that holds a T that is animated over time
// that can be polled for the current value.
template <typename T>
class AnimatedValue : public AnimatedFn<T> {
 public:
  AnimatedValue(T initial, std::shared_ptr<AnimationController> ac)
      : AnimatedFn<T>(ac, [this]() { return t_; },
                      [this](const T& t) { t_ = t; }),
        t_(initial) {}

  const T& Value() const { return t_; }

  // Stops any active animation and jumps directly to the target value.
  void SetValue(const T& t) {
    AnimatedFn<T>::StopAnimation();
    t_ = t;
  }

 private:
  T t_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_ANIMATED_VALUE_H_
