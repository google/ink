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

#ifndef INK_ENGINE_BRUSHES_TIP_SIZE_WORLD_H_
#define INK_ENGINE_BRUSHES_TIP_SIZE_WORLD_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"

namespace ink {

struct TipSizeScreen;

// TipSizeWorld defines the world-coords size of the stroke from the input at a
// given moment. The size can be derived from different sensor inputs, speed or
// a constant value.
// See TipSizeScreen.
struct TipSizeWorld {
 public:
  static TipSizeWorld FromSpeedWithDrag(glm::vec2 radius,
                                        TipSizeWorld previous_size,
                                        float speed_limit, float base_speed,
                                        float speed, float screen_dist,
                                        const Camera& cam);
  static TipSizeWorld FromSpeed(glm::vec2 radius, float speed_limit,
                                float base_speed, float speed);
  static TipSizeWorld FromPressure(glm::vec2 radius, float pressure);
  static TipSizeWorld FromTilt(glm::vec2 radius, float tilt);
  static TipSizeWorld FromOrientation(glm::vec2 radius, float velocity_angle,
                                      float orientation);
  static TipSizeWorld FixedRadius(float radius);

  // Radius of the tip in world coordinates.
  float radius;

  // For non-circular tips (e.g. chisel), the tip size may be determined by a
  // major and minor radius. Note that this doesn't necessarily mean to imply
  // that the tip is an ellipse, just that it has a shape with major and minor
  // axes.
  float radius_minor;

  // Derive a TipSizeScreen from this object.
  TipSizeScreen ToScreen(const Camera& cam) const;

  TipSizeWorld() : TipSizeWorld(0, 0) {}

  // For testing. You probably want the static constructors for real stuff.
  TipSizeWorld(float radius, float radius_minor)
      : radius(radius), radius_minor(radius_minor) {}
};

TipSizeWorld operator*(const TipSizeWorld& tip, float f);
TipSizeWorld operator-(const TipSizeWorld& a, const TipSizeWorld& b);
TipSizeWorld operator+(const TipSizeWorld& a, const TipSizeWorld& b);

}  // namespace ink

#endif  // INK_ENGINE_BRUSHES_TIP_SIZE_WORLD_H_
