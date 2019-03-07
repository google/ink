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

#ifndef INK_ENGINE_GEOMETRY_ALGORITHMS_ENVELOPE_H_
#define INK_ENGINE_GEOMETRY_ALGORITHMS_ENVELOPE_H_

#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/geometry/primitives/triangle.h"

namespace ink {
namespace geometry {

// Finds the smallest axis-aligned rectangle containing the given geometry.
Rect Envelope(const std::vector<glm::vec2>& points);
Rect Envelope(const Triangle& triangle);
Rect Envelope(const RotRect& rot_rect);

// Finds the envelope of the vertices' positions.
Rect Envelope(const std::vector<Vertex>& vertices);

// Finds the envelope of the vertices' texture-coordinates.
Rect TextureEnvelope(const std::vector<Vertex>& vertices);

}  // namespace geometry
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_ALGORITHMS_ENVELOPE_H_
