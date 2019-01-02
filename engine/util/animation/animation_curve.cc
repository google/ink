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

#include "ink/engine/util/animation/animation_curve.h"

#include "ink/engine/geometry/primitives/bezier.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

using glm::vec2;

namespace {

std::unique_ptr<AnimationCurve> MakeEaseInCurve() {
  // Parameters match the "Acceleration" Material animation curve.
  // https://material.io/guidelines/motion/duration-easing.html#duration-easing-natural-easing-curves
  return std::unique_ptr<AnimationCurve>(
      new CubicBezierAnimationCurve(vec2(0.4, 0), vec2(1, 1)));
}

std::unique_ptr<AnimationCurve> MakeEaseOutCurve() {
  // Parameters match the "Deceleration" Material animation curve.
  // https://material.io/guidelines/motion/duration-easing.html#duration-easing-natural-easing-curves
  return std::unique_ptr<AnimationCurve>(
      new CubicBezierAnimationCurve(vec2(0, 0), vec2(0.2, 1)));
}

}  // namespace

std::unique_ptr<AnimationCurve> ReadFromProto(
    const proto::AnimationCurve& proto) {
  switch (proto.type()) {
    case proto::CurveType::EASE_IN_OUT:
    default:
      return std::unique_ptr<AnimationCurve>(new SmoothStepAnimationCurve());
    case proto::CurveType::EASE_IN:
      return MakeEaseInCurve();
    case proto::CurveType::EASE_OUT:
      return MakeEaseOutCurve();
    case proto::CurveType::CUSTOM_CUBIC_BEZIER:
      // CubicBeziers must have exactly 4 params. Crash if we're a debug build,
      // otherwise log and return the default.
      if (proto.params_size() != 4) {
        SLOG(SLOG_ERROR, "Invalid # of cubic params");
        return DefaultAnimationCurve();
      }
      return std::unique_ptr<AnimationCurve>(new CubicBezierAnimationCurve(
          vec2(proto.params(0), proto.params(1)),
          vec2(proto.params(2), proto.params(3))));
  }
}

std::unique_ptr<AnimationCurve> DefaultAnimationCurve() {
  return std::unique_ptr<AnimationCurve>(new SmoothStepAnimationCurve());
}

CubicBezierAnimationCurve::CubicBezierAnimationCurve(glm::vec2 p1,
                                                     glm::vec2 p2) {
  // Unfortunately with this curve formulation it is not feasible to closed form
  // evaluate for arbitrary control points: you have to construct the
  // approximate curve and then search for the correct output progress for a
  // given input progress.
  Bezier b;
  b.MoveTo(glm::vec2(0, 0));
  b.CurveTo(p1, p2, glm::vec2(1, 1));
  auto& polyline = b.Polyline();
  ASSERT(polyline.size() == 1);
  curve_ = polyline[0];
}

float CubicBezierAnimationCurve::Apply(float target) const {
  if (target <= 0) return 0;
  if (target >= 1) return 1;

  // Note: this is doing a linear search to find the output progress, it could
  // do a binary search instead, but it also would require validating the path
  // is actually strictly increasing (which it may not be given certain p0 and
  // p2).
  for (size_t i = 1; i < curve_.size(); ++i) {
    // Each vec2 is a pairs of (input_progress, output_progress).
    glm::vec2 last = curve_[i - 1];
    glm::vec2 cur = curve_[i];

    if (cur.x >= target) {
      // Lerp between the two points that the target is between.
      float amount = util::Normalize(last.x, cur.x, target);
      return util::Lerp(last.y, cur.y, amount);
    }
  }

  ASSERT(false);
  return 0;
}

}  // namespace ink
