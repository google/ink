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

#ifndef INK_ENGINE_GEOMETRY_MESH_VERTEX_TYPES_H_
#define INK_ENGINE_GEOMETRY_MESH_VERTEX_TYPES_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "third_party/absl/types/variant.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/rect.h"

namespace ink {

enum class VertFormat {
  // The xy-coordinates are not packed, each component has its own float.
  x32y32,
  // The xy-coordinates are packed into a single float.
  x12y12,
  // The x-coordinate, alpha-component, and red-component are packed into one
  // float, and the y-coordinate, green-component, and blue-component are packed
  // into another.
  x11a7r6y11g7b6,
  // The x-coordinate, alpha-component, and red-component are packed into one
  // float, the y-coordinate, green-component, and blue-component are packed
  // into another, and the texture uv-coordinates are packed in a third.
  x11a7r6y11g7b6u12v12,
};

inline std::string VertFormatName(VertFormat f) {
  switch (f) {
    case VertFormat::x32y32:
      return "x32y32";
    case VertFormat::x12y12:
      return "x12y12";
    case VertFormat::x11a7r6y11g7b6:
      return "x11a7r6y11g7b6";
    case VertFormat::x11a7r6y11g7b6u12v12:
      return "x11a7r6y11g7b6u12v12";
  }
}

// A list of vertices stored in a compressed format, as specified by "format".
class PackedVertList {
 public:

  // Uses the x32y32 format
  PackedVertList();

  explicit PackedVertList(VertFormat format);

  // Removes all elements from the list, leaves the format unchanged.
  void Clear();

  VertFormat GetFormat() const;

  // The number of vertices in the list
  uint32_t size() const;

  // Be like a vector.
  bool empty() const;

  // The number of bytes used to store each vertex in the list
  uint32_t VertexSizeBytes() const;

  // A raw ptr to the first element of list being used to hold data (either
  // floats or vec2s). Assumes size of the list > 0.
  const void* Ptr() const;

  // Unpacks vertex at given index into provided vertex struct.
  void UnpackVertex(uint32_t idx, Vertex* vertex) const;

  // This transform maps from the packed texture uv-coordinates to the unpacked
  // texture uv-coordinates. It is only used for VertexType::x12y12u12v12.
  const glm::mat4& PackedUvToUvTransform() const { return packed_uv_to_uv_; }

  // transform is expected to be the result of CalcTransformForFormat(), called
  // with the same vertices and format. The MBR of the packed vertices will be
  // the rectangle from (0, 0) to (m, m), where m is the maximum coordinate for
  // the format.
  static PackedVertList PackVerts(const std::vector<Vertex>& verts,
                                  const glm::mat4& transform,
                                  VertFormat to_format);

  static uint32_t CalcRequiredPrecision(VertFormat fmt);
  static glm::mat4 CalcTransformForFormat(Rect mesh_envelope,
                                          VertFormat to_format);
  static Rect CalcTargetEnvelopeForFormat(VertFormat format);
  static float GetMaxCoordinateForFormat(VertFormat format);

 private:
  // These helpers are used by PackVerts to construct the data vectors for the
  // various formats.
  static std::vector<float> PackVertsX12Y12(const std::vector<Vertex>& verts,
                                            const glm::mat4& transform,
                                            float max_coord);
  static std::vector<glm::vec2> PackVertsX32Y32(
      const std::vector<Vertex>& verts, const glm::mat4& transform,
      float max_coord);
  static std::vector<glm::vec2> PackVertsX11A7R6Y11G7B6(
      const std::vector<Vertex>& verts, const glm::mat4& transform,
      float max_coord);
  static std::vector<glm::vec3> PackVertsX11A7R6Y11G7B6U12V12(
      const std::vector<Vertex>& verts, const glm::mat4& transform,
      float max_coord, glm::mat4* packed_uv_to_uv);

  static glm::mat4 CalculateUvToPackedUvTransform(Rect uv_bounds,
                                                  float uv_max_coord);

  // These fetch the data as a vector of the given type. This will check-fail if
  // the variant does not hold the correct type, so you must always check the
  // format first to determine which type to fetch.
  const std::vector<float>& FloatData() const;
  const std::vector<glm::vec2>& Vec2Data() const;
  const std::vector<glm::vec3>& Vec3Data() const;

  VertFormat format_;
  absl::variant<std::vector<float>, std::vector<glm::vec2>,
                std::vector<glm::vec3>>
      data_;

  glm::mat4 packed_uv_to_uv_{1};

  friend class MeshVBOProvider;
  friend class PackedVertListTestHelper;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_VERTEX_TYPES_H_
