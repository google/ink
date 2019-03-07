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

#include "ink/engine/geometry/mesh/vertex_types.h"

#include <netinet/in.h>
#include <cstddef>
#include <limits>

#include "third_party/absl/strings/substitute.h"
#include "third_party/absl/types/variant.h"
#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/float_pack.h"

namespace ink {

using glm::vec2;
using glm::vec3;
using glm::vec4;

PackedVertList::PackedVertList() : PackedVertList(VertFormat::x32y32) {}

PackedVertList::PackedVertList(VertFormat format) : format_(format) {}

void PackedVertList::Clear() {
  switch (format_) {
    case VertFormat::x11a7r6y11g7b6u12v12:
      data_ = std::vector<glm::vec3>();
      break;
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      data_ = std::vector<glm::vec2>();
      break;
    case VertFormat::x12y12:
      data_ = std::vector<float>();
  }
}

uint32_t PackedVertList::size() const {
  switch (format_) {
    case VertFormat::x11a7r6y11g7b6u12v12:
      return Vec3Data().size();
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return Vec2Data().size();
    case VertFormat::x12y12:
      return FloatData().size();
  }
}

bool PackedVertList::empty() const {
  switch (format_) {
    case VertFormat::x11a7r6y11g7b6u12v12:
      return Vec3Data().empty();
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return Vec2Data().empty();
    case VertFormat::x12y12:
      return FloatData().empty();
  }
}

VertFormat PackedVertList::GetFormat() const { return format_; }

uint32_t PackedVertList::VertexSizeBytes() const {
  switch (format_) {
    case VertFormat::x11a7r6y11g7b6u12v12:
      return sizeof(glm::vec3);
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return sizeof(glm::vec2);
    case VertFormat::x12y12:
      return sizeof(float);
  }
}

const void* PackedVertList::Ptr() const {
  EXPECT(size() > 0);
  switch (format_) {
    case VertFormat::x11a7r6y11g7b6u12v12:
      return &Vec3Data()[0];
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return &Vec2Data()[0];
    case VertFormat::x12y12:
      return &FloatData()[0];
  }
}

float PackedVertList::GetMaxCoordinateForFormat(VertFormat format) {
  switch (format) {
    case VertFormat::x32y32:
    case VertFormat::x12y12:
      return 4095;  // 2ˆ12 - 1
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x11a7r6y11g7b6u12v12:
      return 2047;  // 2ˆ11 - 1
  }
}

Rect PackedVertList::CalcTargetEnvelopeForFormat(VertFormat format) {
  float max_coord = GetMaxCoordinateForFormat(format);
  return Rect(0, 0, max_coord, max_coord);
}

void PackedVertList::UnpackVertex(uint32_t idx, Vertex* vertex) const {
  EXPECT(idx < size());
  switch (format_) {
    case VertFormat::x11a7r6y11g7b6:
      UnpackColorAndPosition(Vec2Data()[idx], &vertex->color,
                             &vertex->position);
      return;
    case VertFormat::x32y32:
      vertex->position.x = Vec2Data()[idx].x;
      vertex->position.y = Vec2Data()[idx].y;
      return;
    case VertFormat::x12y12: {
      float packed_float = FloatData()[idx];
      vertex->position = UnpackPosition(packed_float);
      return;
    }
    case VertFormat::x11a7r6y11g7b6u12v12: {
      glm::vec3 packed_vec3 = Vec3Data()[idx];
      UnpackColorAndPosition(glm::vec2(packed_vec3), &vertex->color,
                             &vertex->position);
      vertex->texture_coords =
          geometry::Transform(UnpackPosition(packed_vec3.z), packed_uv_to_uv_);
      return;
    }
  }
}

PackedVertList PackedVertList::PackVerts(const std::vector<Vertex>& verts,
                                         const glm::mat4& transform,
                                         VertFormat to_format) {
  PackedVertList res(to_format);

  float max_coord = GetMaxCoordinateForFormat(to_format);
  switch (to_format) {
    case VertFormat::x12y12:
      res.data_ = PackVertsX12Y12(verts, transform, max_coord);
      break;
    case VertFormat::x32y32:
      res.data_ = PackVertsX32Y32(verts, transform, max_coord);
      break;
    case VertFormat::x11a7r6y11g7b6:
      res.data_ = PackVertsX11A7R6Y11G7B6(verts, transform, max_coord);
      break;
    case VertFormat::x11a7r6y11g7b6u12v12:
      res.data_ = PackVertsX11A7R6Y11G7B6U12V12(verts, transform, max_coord,
                                                &res.packed_uv_to_uv_);
      break;
  }

  return res;
}

std::vector<float> PackedVertList::PackVertsX12Y12(
    const std::vector<Vertex>& verts, const glm::mat4& transform,
    float max_coord) {
  std::vector<float> floats(verts.size());
  for (size_t i = 0; i < verts.size(); i++) {
    // The vertices are rounded and clamped in PackPosition().
    floats[i] = PackPosition(geometry::Transform(verts[i].position, transform));
  }
  return floats;
}

std::vector<glm::vec2> PackedVertList::PackVertsX32Y32(
    const std::vector<Vertex>& verts, const glm::mat4& transform,
    float max_coord) {
  std::vector<glm::vec2> vec2s(verts.size());
  for (size_t i = 0; i < verts.size(); i++) {
    // The vertices may be outside the target bounds by ± epsilon after the
    // transform is applied, so we clamp them into the desired range.
    vec2s[i] = util::Clamp0N(max_coord,
                             geometry::Transform(verts[i].position, transform));
  }
  return vec2s;
}

std::vector<glm::vec2> PackedVertList::PackVertsX11A7R6Y11G7B6(
    const std::vector<Vertex>& verts, const glm::mat4& transform,
    float max_coord) {
  std::vector<glm::vec2> vec2s(verts.size());
  for (size_t i = 0; i < verts.size(); i++) {
    // The vertices are rounded and clamped in PackColorAndPosition().
    vec2s[i] = PackColorAndPosition(
        verts[i].color, geometry::Transform(verts[i].position, transform));
  }
  return vec2s;
}

std::vector<glm::vec3> PackedVertList::PackVertsX11A7R6Y11G7B6U12V12(
    const std::vector<Vertex>& verts, const glm::mat4& transform,
    float max_coord, glm::mat4* packed_uv_to_uv) {
  // The texture uv-coordinates are packed into a single float (the z-component
  // of the vec3), in the same way that x12y12 vertices are packed.
  auto uv_to_packed_uv = CalculateUvToPackedUvTransform(
      geometry::TextureEnvelope(verts),
      GetMaxCoordinateForFormat(VertFormat::x12y12));
  *packed_uv_to_uv = glm::inverse(uv_to_packed_uv);
  std::vector<glm::vec3> vec3s(verts.size());
  for (size_t i = 0; i < verts.size(); i++) {
    // The vertices are rounded and clamped in PackColorAndPosition() and
    // PackPosition().
    vec3s[i] = {
        PackColorAndPosition(verts[i].color,
                             geometry::Transform(verts[i].position, transform)),
        PackPosition(
            geometry::Transform(verts[i].texture_coords, uv_to_packed_uv))};
  }
  return vec3s;
}

// Returns the minimum number of bits required to store material for
// the format.
uint32_t PackedVertList::CalcRequiredPrecision(VertFormat fmt) {
  uint32_t precision_bits = 12;
  if (fmt == VertFormat::x11a7r6y11g7b6) {
    precision_bits = 11;
  }
  return precision_bits;
}

glm::mat4 PackedVertList::CalcTransformForFormat(Rect mesh_envelope,
                                                 VertFormat to_format) {
  Rect target_envelope = CalcTargetEnvelopeForFormat(to_format);

  vec2 scale = vec2(target_envelope.Width() / mesh_envelope.Width(),
                    target_envelope.Height() / mesh_envelope.Height());

  glm::mat4 m{1};
  m = glm::translate(m, vec3(target_envelope.Center(), 0));
  m = glm::scale(m, vec3(scale, 1));
  m = glm::translate(m, vec3(-mesh_envelope.Center(), 0));

  return m;
}

glm::mat4 PackedVertList::CalculateUvToPackedUvTransform(Rect uv_bounds,
                                                         float uv_max_coord) {
  glm::mat4 uv_to_packed_uv{1};
  if (uv_bounds.Area() != 0) {
    uv_to_packed_uv =
        uv_bounds.CalcTransformTo(Rect({0, 0}, {uv_max_coord, uv_max_coord}));
  } else {
    glm::vec2 dim = uv_bounds.Dim();
    uv_to_packed_uv =
        glm::translate(glm::mat4{1}, glm::vec3{-uv_bounds.from, 0});
    if (dim.x != 0) {
      uv_to_packed_uv =
          glm::scale(glm::mat4{1}, glm::vec3{uv_max_coord / dim.x, 1, 1}) *
          uv_to_packed_uv;
    } else if (dim.y != 0) {
      uv_to_packed_uv =
          glm::scale(glm::mat4{1}, glm::vec3{1, uv_max_coord / dim.y, 1}) *
          uv_to_packed_uv;
    }
  }
  return uv_to_packed_uv;
}

void ExpectLittleEndianIEEE754() {
  static_assert(sizeof(float) == 4,
                "Only 4 byte floats are supported for serialization");
  static_assert(std::numeric_limits<float>::is_iec559,
                "Only IEEE 754 floating point is supported for serialization");
  static bool s_endianess_known = false;
  static bool s_little_endian = false;
  if (!s_endianess_known) {
    s_little_endian = htonl(42) != 42;
    EXPECT(
        s_little_endian);  // only little-endian architectures supported for now
  }
}

const std::vector<float>& PackedVertList::FloatData() const {
  return absl::get<std::vector<float>>(data_);
}

const std::vector<glm::vec2>& PackedVertList::Vec2Data() const {
  return absl::get<std::vector<glm::vec2>>(data_);
}

const std::vector<glm::vec3>& PackedVertList::Vec3Data() const {
  return absl::get<std::vector<glm::vec3>>(data_);
}

}  // namespace ink
