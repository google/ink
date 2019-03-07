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

#ifndef INK_ENGINE_REALTIME_MODIFIERS_TILED_TEXTURE_H_
#define INK_ENGINE_REALTIME_MODIFIERS_TILED_TEXTURE_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/util/funcs/piecewise_interpolator.h"

namespace ink {

class TiledTextureModifier : public LineModifier {
 public:
  explicit TiledTextureModifier(const glm::vec4& rgba,
                                const glm::mat4& texture_transform,
                                const string texture_uri);
  void OnAddVert(Vertex* vert, glm::vec2 center_pt, float radius,
                 float pressure) override;
  ShaderType GetShaderType() override { return ShaderType::TexturedVertShader; }

 private:
  static LineModParams MakeLineModParams(string texture_uri);
  glm::mat4 texture_transform_{1};
  PiecewiseInterpolator opacity_interpolator_;
};

}  // namespace ink
#endif  // INK_ENGINE_REALTIME_MODIFIERS_TILED_TEXTURE_H_
