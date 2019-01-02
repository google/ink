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

#ifndef INK_ENGINE_REALTIME_MODIFIERS_LINE_ANIMATION_H_
#define INK_ENGINE_REALTIME_MODIFIERS_LINE_ANIMATION_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

class ILineAnimation {
 public:
  virtual ~ILineAnimation() {}
  virtual void SetupNewLine(const input::InputData& data,
                            const Camera& camera) = 0;
  virtual void ApplyToVert(Vertex* vert, glm::vec2 center_pt, float radius,
                           DurationS time_since_tdown,
                           const std::vector<Vertex>& line,
                           const LineModParams& line_mod_params) = 0;
};

// Animates a linear color blend and dilation
class LinearPathAnimation : public ILineAnimation {
 public:
  explicit LinearPathAnimation(const glm::vec4& rgba,
                               const glm::vec4& rgba_from,
                               DurationS rgba_seconds, float dilation_from,
                               DurationS dilation_seconds);
  void SetupNewLine(const input::InputData& data,
                    const Camera& camera) override {}
  void ApplyToVert(Vertex* vert, glm::vec2 center_pt, float radius,
                   DurationS time_since_tdown, const std::vector<Vertex>& line,
                   const LineModParams& line_mod_params) override;

 protected:
  glm::vec4 rgba_{0, 0, 0, 0};
  glm::vec4 rgba_from_{0, 0, 0, 0};

  double rgba_seconds_;
  float dilation_from_;
  double dilation_seconds_;
};

}  // namespace ink
#endif  // INK_ENGINE_REALTIME_MODIFIERS_LINE_ANIMATION_H_
