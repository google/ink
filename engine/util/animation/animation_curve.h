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

#ifndef INK_ENGINE_UTIL_ANIMATION_ANIMATION_CURVE_H_
#define INK_ENGINE_UTIL_ANIMATION_ANIMATION_CURVE_H_

#include <functional>
#include <memory>

#include "ink/engine/geometry/primitives/bezier.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/proto/animations_portable_proto.pb.h"

namespace ink {

// An animation curve remaps the linear range [0..1] to control forward progress
// of an animation over time.
// Note that as an invariant, Apply(0) must be 0 and Apply(1) must be 1. The
// change of the return value is allowed to be outside the range [0..1] for
// the intermediate values however.
class AnimationCurve {
 public:
  virtual ~AnimationCurve() {}
  virtual float Apply(float progress) const = 0;
};

std::unique_ptr<AnimationCurve> DefaultAnimationCurve();

std::unique_ptr<AnimationCurve> ReadFromProto(
    const proto::AnimationCurve& proto);

class LinearAnimationCurve : public AnimationCurve {
 public:
  float Apply(float progress) const override { return progress; }
};

class SmoothStepAnimationCurve : public AnimationCurve {
 public:
  float Apply(float progress) const override {
    return util::Smoothstep(0.0f, 1.0f, progress);
  }
};

class CubicBezierAnimationCurve : public AnimationCurve {
 public:
  // The cubic bezier is the industry standard formulation where the control
  // points are tuples of (linear_progress, output_progress), with the fixed
  // values of p0 = (0, 0) and p3 = (1, 1).
  CubicBezierAnimationCurve(glm::vec2 p1, glm::vec2 p2);

  float Apply(float progress) const override;

 private:
  std::vector<glm::vec2> curve_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_ANIMATION_CURVE_H_
