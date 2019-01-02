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

#include "ink/engine/geometry/mesh/vertex.h"

#include <ostream>

#include "third_party/absl/strings/str_format.h"

namespace ink {
bool Vertex::operator==(const Vertex& other) const {
  return other.position == position && other.color == color &&
         other.texture_coords == texture_coords &&
         other.position_from == position_from &&
         other.color_from == color_from &&
         other.texture_coords_from == texture_coords_from &&
         other.position_timings == position_timings &&
         other.color_timings == color_timings &&
         other.texture_timings == texture_timings;
}

Vertex Vertex::operator*(float scalar) const {
  Vertex v = *this;
  v.position *= scalar;
  v.color *= scalar;
  v.texture_coords *= scalar;

  v.position_from *= scalar;
  v.color_from *= scalar;
  v.texture_coords_from *= scalar;

  v.position_timings *= scalar;
  v.color_timings *= scalar;
  v.texture_timings *= scalar;
  return v;
}

Vertex Vertex::operator/(float scalar) const { return *this * (1.0f / scalar); }

Vertex Vertex::operator-(const Vertex& other) const {
  return *this + (other * -1.0f);
}

Vertex Vertex::operator+(const Vertex& other) const {
  Vertex v = *this;
  v.position += other.position;
  v.color += other.color;
  v.texture_coords += other.texture_coords;

  v.position_from += other.position_from;
  v.color_from += other.color_from;
  v.texture_coords_from += other.texture_coords_from;

  v.position_timings += other.position_timings;
  v.color_timings += other.color_timings;
  v.texture_timings += other.texture_timings;
  return v;
}

std::ostream& operator<<(std::ostream& oss, const Vertex& vert) {
  oss << absl::StrFormat("pos:(%.2f, %.2f), clr:(%.1f,%.1f,%.1f,%.1f)",
                         vert.position.x, vert.position.y, vert.color.r,
                         vert.color.g, vert.color.b, vert.color.w);
  return oss;
}

}  // namespace ink
