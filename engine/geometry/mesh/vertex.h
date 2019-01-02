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

#ifndef INK_ENGINE_GEOMETRY_MESH_VERTEX_H_
#define INK_ENGINE_GEOMETRY_MESH_VERTEX_H_

#include <iosfwd>
#include <string>

#include "third_party/absl/strings/str_format.h"
#include "third_party/glm/glm/glm.hpp"

namespace ink {

// NOTE: InterleavedAttributeSet and VBO.h have dependencies on the
// data layout of this struct! Beware making changes.
struct Vertex {
  glm::vec2 position{0, 0};
  glm::vec4 color{0, 0, 0, 0};
  glm::vec2 texture_coords{0, 0};

  // Animation data
  glm::vec2 position_from{0, 0};
  glm::vec4 color_from{0, 0, 0, 0};
  glm::vec2 texture_coords_from{0, 0};

  // Animation times, measured in seconds since ShaderMetadata::InitTime(). x is
  // the start time, y is the end time.
  glm::vec2 position_timings{0, 0};
  glm::vec2 color_timings{0, 0};
  glm::vec2 texture_timings{0, 0};

  Vertex() {}
  explicit Vertex(glm::vec2 v) : position(v) {}
  explicit Vertex(glm::vec3 v) : position(v) {}
  explicit Vertex(glm::vec4 v) : position(v) {}
  Vertex(glm::vec2 v, glm::vec4 clr) : position(v), color(clr) {}
  Vertex(int x, int y) {
    position.x = static_cast<float>(x);
    position.y = static_cast<float>(y);
  }

  Vertex(float x, float y) {
    position.x = x;
    position.y = y;
  }

  Vertex(double x, double y) {
    position.x = static_cast<float>(x);
    position.y = static_cast<float>(y);
  }

  std::string ToString() const {
    return absl::StrFormat("(%.2f, %.2f)", position.x, position.y);
  }

  template <typename T, typename V>
  static Vertex Mix(T verts_begin, V weights_begin, size_t n) {
    Vertex res;
    for (size_t i = 0; i < n; i++) {
      res = res + (verts_begin[i] * weights_begin[i]);
    }
    return res;
  }

  bool operator==(const Vertex& other) const;
  bool operator!=(const Vertex& other) const { return !(other == *this); }
  Vertex operator*(float scalar) const;
  Vertex operator/(float scalar) const;
  Vertex operator+(const Vertex& other) const;
  Vertex operator-(const Vertex& other) const;

  friend std::ostream& operator<<(std::ostream& oss, const Vertex& vert);
};
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_MESH_VERTEX_H_
