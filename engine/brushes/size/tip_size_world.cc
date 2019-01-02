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

#include "ink/engine/brushes/size/tip_size_world.h"

#include "ink/engine/brushes/size/tip_size_screen.h"
#include "ink/engine/util/funcs/piecewise_interpolator.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

using glm::vec2;

float RadiusFromSpeed(vec2 radius, float speed_limit, float base_speed,
                      float speed) {
  auto spdfloor = std::max(0.0f, speed - base_speed);
  float normspeed = spdfloor / speed_limit;
  auto gstep = [](float from, float to, float amount) -> float {
    // Gompertz function
    constexpr float e = 2.71828183f;
    float y = std::pow(e, -4.0f * std::pow(e, -4.0f * amount));
    return y * (to - from) + from;
  };
  return gstep(radius.x, radius.y, normspeed);
}

float DragRadius(float target_radius, float previous_radius, float screen_dist,
                 const Camera& cam) {
  // limit d(radius)/d(pos)
  if (screen_dist < 1) {
    target_radius = previous_radius;
  } else {
    // World to screen
    auto wts = [&cam](float f) {
      return cam.ConvertDistance(f, DistanceType::kWorld,
                                 DistanceType::kScreen);
    };
    auto d_screen_radius = wts(previous_radius) - wts(target_radius);

    auto coverage = util::Normalize(0.0f, 30.0f, std::abs(d_screen_radius));
    coverage = 1 - util::Clamp01(coverage);

    auto max_dscreen = util::Lerp(0.1f, 0.15f, coverage);
    auto d_screen = d_screen_radius / screen_dist;
    if (std::abs(d_screen) > max_dscreen) {
      auto mult = -d_screen >= 0 ? 1 : -1;
      auto target_screen =
          mult * max_dscreen * screen_dist + wts(previous_radius);
      auto n_radius = cam.ConvertDistance(target_screen, DistanceType::kScreen,
                                          DistanceType::kWorld);
      target_radius = n_radius;
    }
  }

  float drag;
  if (target_radius > previous_radius) {
    drag = 0.95f;
  } else {
    drag = 0.85f;
  }
  target_radius = (previous_radius * (drag) + (target_radius * (1 - drag)));
  return target_radius;
}

TipSizeWorld TipSizeWorld::FromSpeedWithDrag(
    vec2 radius, TipSizeWorld previous_size, float speed_limit,
    float base_speed, float speed, float screen_dist, const Camera& cam) {
  float r = DragRadius(RadiusFromSpeed(radius, speed_limit, base_speed, speed),
                       previous_size.radius, screen_dist, cam);
  return TipSizeWorld({r, r});
}

TipSizeWorld TipSizeWorld::FromSpeed(vec2 radius, float speed_limit,
                                     float base_speed, float speed) {
  float r = RadiusFromSpeed(radius, speed_limit, base_speed, speed);
  return TipSizeWorld({r, r});
}

TipSizeWorld TipSizeWorld::FromPressure(vec2 radius, float pressure) {
  // Multiply the base radius by this coefficient for varying pressure values.
  PiecewiseInterpolator pressure_to_radius(
      {vec2(0, 1), vec2(0.2, 1.3), vec2(0.8, 1.3), vec2(1, 3)});
  float r = (pressure < 0) ? radius.x
                           : pressure_to_radius.GetValue(pressure) * radius.x;
  return TipSizeWorld({r, r});
}

TipSizeWorld TipSizeWorld::FromTilt(vec2 radius, float tilt) {
  return TipSizeWorld(
      {util::Lerp(radius.x, radius.y, tilt / (M_PI / 2)), radius.x});
}

TipSizeWorld TipSizeWorld::FromOrientation(vec2 radius, float velocity_angle,
                                           float orientation) {
  float delta = orientation - velocity_angle;

  return TipSizeWorld(
      {util::Lerp(radius.x, radius.y, fabs(sin(delta))), radius.x});
}

TipSizeWorld TipSizeWorld::FixedRadius(float radius) {
  return TipSizeWorld({radius, radius});
}

TipSizeScreen TipSizeWorld::ToScreen(const Camera& cam) const {
  TipSizeScreen ret;
  ret.radius =
      cam.ConvertDistance(radius, DistanceType::kWorld, DistanceType::kScreen);
  ret.radius_minor = cam.ConvertDistance(radius_minor, DistanceType::kWorld,
                                         DistanceType::kScreen);
  return ret;
}

TipSizeWorld operator*(const TipSizeWorld& tip, float f) {
  return TipSizeWorld(tip.radius * f, tip.radius_minor * f);
}

TipSizeWorld operator-(const TipSizeWorld& a, const TipSizeWorld& b) {
  return TipSizeWorld(a.radius - b.radius, a.radius_minor - b.radius_minor);
}

TipSizeWorld operator+(const TipSizeWorld& a, const TipSizeWorld& b) {
  return TipSizeWorld(a.radius + b.radius, a.radius_minor + b.radius_minor);
}

}  // namespace ink
