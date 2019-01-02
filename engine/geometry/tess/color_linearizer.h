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

#ifndef INK_ENGINE_GEOMETRY_TESS_COLOR_LINEARIZER_H_
#define INK_ENGINE_GEOMETRY_TESS_COLOR_LINEARIZER_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/mesh_triangle.h"
namespace ink {

class ColorLinearizer {
 public:
  explicit ColorLinearizer(Mesh* mesh);
  void LinearizeCombinedVerts();
  void LinearizeAllVerts();

 private:
  void Initdata();
  void Pass(std::vector<uint16_t>::iterator from,
            std::vector<uint16_t>::iterator to, float amt);

 private:
  Mesh* mesh_;
  MeshTriangle* tris_;
  uint32_t ntris_;
  std::unordered_multimap<uint16_t, uint16_t> pttoseg_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_TESS_COLOR_LINEARIZER_H_
