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
#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/float_pack.h"

namespace ink {

using glm::vec2;
using glm::vec3;
using glm::vec4;

PackedVertList::PackedVertList() : PackedVertList(VertFormat::x32y32) {}

PackedVertList::PackedVertList(VertFormat format) : format(format) {}

void PackedVertList::Clear() {
  // swap to delete allocated capacity
  std::vector<float>().swap(floats);
  std::vector<vec2>().swap(vec2s);
  std::vector<Vertex>().swap(uncompressed_verts);
}

uint32_t PackedVertList::size() const {
  switch (format) {
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return vec2s.size();
    case VertFormat::x12y12:
      return floats.size();
    case VertFormat::uncompressed:
      return uncompressed_verts.size();
  }
  RUNTIME_ERROR("Unknown format $0", static_cast<int>(format));
}

bool PackedVertList::empty() const {
  switch (format) {
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return vec2s.empty();
    case VertFormat::x12y12:
      return floats.empty();
    case VertFormat::uncompressed:
      return uncompressed_verts.empty();
  }
  RUNTIME_ERROR("Unknown format $0", static_cast<int>(format));
}

VertFormat PackedVertList::GetFormat() const { return format; }

uint32_t PackedVertList::VertexSizeBytes() const {
  switch (format) {
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return sizeof(vec2);
    case VertFormat::x12y12:
      return sizeof(float);
    case VertFormat::uncompressed:
      return sizeof(Vertex);
  }
  RUNTIME_ERROR("Unknown format $0", static_cast<int>(format));
}

const void* PackedVertList::Ptr() const {
  EXPECT(size() > 0);
  switch (format) {
    case VertFormat::x11a7r6y11g7b6:
    case VertFormat::x32y32:
      return &vec2s[0];
    case VertFormat::x12y12:
      return &floats[0];
    case VertFormat::uncompressed:
      return &uncompressed_verts[0];
  }
  RUNTIME_ERROR("Unknown format $0", static_cast<int>(format));
}

float PackedVertList::GetMaxCoordinateForFormat(VertFormat format) {
  switch (format) {
    case VertFormat::x32y32:
    case VertFormat::uncompressed:
    case VertFormat::x12y12:
      return 4095;  // 2ˆ12 - 1
    case VertFormat::x11a7r6y11g7b6:
      return 2047;  // 2ˆ11 - 1
    default:
      EXPECT(false);
      return 0;
  }
}

Rect PackedVertList::CalcTargetEnvelopeForFormat(VertFormat format) {
  float max_coord = GetMaxCoordinateForFormat(format);
  return Rect(0, 0, max_coord, max_coord);
}

void PackedVertList::UnpackVertex(uint32_t idx, Vertex* vertex) const {
  EXPECT(idx < size());
  switch (format) {
    case VertFormat::x11a7r6y11g7b6:
      UnpackColorAndPosition(vec2s[idx], &vertex->color, &vertex->position);
      return;
    case VertFormat::x32y32:
      vertex->position.x = vec2s[idx].x;
      vertex->position.y = vec2s[idx].y;
      return;
    case VertFormat::x12y12: {
      float packed_float = floats[idx];
      vertex->position = UnpackPosition(packed_float);
      return;
    }
    case VertFormat::uncompressed:
      *vertex = uncompressed_verts[idx];
      return;
  }
  RUNTIME_ERROR("Unknown format $0", static_cast<int>(format));
}

PackedVertList PackedVertList::PackVerts(const std::vector<Vertex>& verts,
                                         const glm::mat4& transform,
                                         VertFormat to_format) {
  PackedVertList res(to_format);

  auto trfm = [&transform](const Vertex& v) -> vec2 {
    auto v4 = transform * vec4(v.position.x, v.position.y, 1, 1);
    return vec2(v4);
  };

  // Vertices in x32y32 and uncompressed formats may be outside the target
  // bounds by +/- epsilon after the transform is applied, so we clamp them into
  // the desired range.
  // Vertices in x12y12 and x11a7r6y11g7b6 formats are rounded and clamped in
  // PackPosition(), and PackColorAndPosition().
  float max_coord = GetMaxCoordinateForFormat(to_format);
  switch (to_format) {
    case VertFormat::x32y32:
      res.vec2s.resize(verts.size());
      for (size_t i = 0; i < verts.size(); i++) {
        res.vec2s[i] = util::Clamp0N(max_coord, trfm(verts[i]));
      }
      break;

    case VertFormat::x12y12:
      res.floats.resize(verts.size());
      for (size_t i = 0; i < verts.size(); i++) {
        res.floats[i] = PackPosition(trfm(verts[i]));
      }
      break;

    case VertFormat::x11a7r6y11g7b6:
      res.vec2s.resize(verts.size());
      for (size_t i = 0; i < verts.size(); i++) {
        res.vec2s[i] = PackColorAndPosition(verts[i].color, trfm(verts[i]));
      }
      break;
    case VertFormat::uncompressed:
      res.uncompressed_verts.resize(verts.size());
      for (size_t i = 0; i < verts.size(); i++) {
        res.uncompressed_verts[i] = verts[i];
        res.uncompressed_verts[i].position =
            util::Clamp0N(max_coord, trfm(verts[i]));
      }
      break;
    default:
      RUNTIME_ERROR("Unknown format $0", static_cast<int>(to_format));
  }

  return res;
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

void PackedVertList::CheckOnlyOneVectorHasData() const {
  int non_empty_lists = 0;
  non_empty_lists += (!uncompressed_verts.empty());
  non_empty_lists += (!floats.empty());
  non_empty_lists += (!vec2s.empty());
  EXPECT(non_empty_lists <= 1);
}
}  // namespace ink
