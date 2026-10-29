// Copyright 2026 Google LLC
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

#ifndef INK_GEOMETRY_INTERNAL_OUTLINE_TRIANGULATION_H_
#define INK_GEOMETRY_INTERNAL_OUTLINE_TRIANGULATION_H_

#include <array>
#include <cstdint>
#include <vector>

#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/point.h"

namespace ink::geometry_internal {

// A representation of a triangulation, consisting of a list of `vertices` and
// `triangles` represented by triplets of indices of vertices.
struct Triangulation {
  std::vector<Point> vertices;
  std::vector<std::array<uint32_t, 3>> triangles;
};

// Returns a triangulation of the given shape.
Triangulation Triangulate(const ShapeOutline& shape);

}  // namespace ink::geometry_internal

#endif  // INK_GEOMETRY_INTERNAL_OUTLINE_TRIANGULATION_H_
