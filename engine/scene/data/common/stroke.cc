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

#include "ink/engine/scene/data/common/stroke.h"

#include <cstdint>

#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {

using status::InvalidArgument;

namespace {

// Update this on changes that break compatibility. If the client
// encounters ink with a newer version, it alerts the user to update
// their app.
//
// Version 0: Encoded strokes in Element::polygon and
// Element::polygon_bytes in the proto. (NO LONGER SUPPORTED)
// Version 1: Encodes strokes using OpenCTM
constexpr uint32_t kSerializerVersion = 1;

// If successful, returns true and sets ShaderType. False otherwise.
Status DeserializeShaderTypeFromStroke(const ink::proto::Stroke& stroke,
                                       ShaderType* shader_type) {
  if (!stroke.has_shader_type()) {
    return InvalidArgument("Expected a shader_type in proto -- not set. ");
  }
  switch (stroke.shader_type()) {
    case ink::proto::VERTEX_COLORED:
      *shader_type = ColoredVertShader;
      break;
    case ink::proto::SOLID_COLORED:
      *shader_type = SingleColorShader;
      break;
    case ink::proto::ERASE:
      *shader_type = EraseShader;
      break;
    case ink::proto::VERTEX_TEXTURED:
      *shader_type = TexturedVertShader;
      break;
    default:
      return InvalidArgument("Unknown shader type: $0",
                             static_cast<int>(stroke.shader_type()));
  }
  return OkStatus();
}
}  // namespace

Stroke::Stroke() : Stroke(kInvalidUUID, NoShader, glm::mat4{1}) {}

Stroke::Stroke(const UUID& uuid, ShaderType shader_type, glm::mat4 obj_to_world)
    : uuid_(uuid), obj_to_world_(obj_to_world), shader_type_(shader_type) {}

// static
Status Stroke::ReadFromProto(const ink::proto::ElementBundle& unsafe_bundle,
                             Stroke* s) {
  ink::ElementBundle cxxbundle;
  if (ink::util::ReadFromProto(unsafe_bundle, &cxxbundle)) {
    return ink::util::ReadFromProto(cxxbundle, s);
  }
  return OkStatus();
}

// static
Status Stroke::ReadFromProto(const ink::ElementBundle& unsafe_bundle,
                             Stroke* s) {
  if (!unsafe_bundle.unsafe_element().has_stroke()) {
    return InvalidArgument("bundle element does not have a stroke");
  }
  ShaderType shader_type;
  INK_RETURN_UNLESS(DeserializeShaderTypeFromStroke(
      unsafe_bundle.unsafe_element().stroke(), &shader_type));
  size_t num_meshes = unsafe_bundle.unsafe_element().stroke().lod_size();
  INK_RETURN_UNLESS(BoundsCheckExInc(num_meshes, 0u, 10u));
  glm::mat4 obj_to_world{1};
  INK_RETURN_UNLESS(ink::util::ReadFromProto(unsafe_bundle.unsafe_transform(),
                                             &obj_to_world));
  for (const auto& lod : unsafe_bundle.unsafe_element().stroke().lod()) {
    if (!(lod.has_ctm_blob() || lod.has_draco_blob())) {
      return InvalidArgument("stroke missing encoded mesh");
    }
  }
  s->uuid_ = unsafe_bundle.safe_uuid();
  s->stroke_ = unsafe_bundle.unsafe_element().stroke();
  s->obj_to_world_ = obj_to_world;
  s->shader_type_ = shader_type;
  return OkStatus();
}

Status Stroke::GetInputPoints(InputPoints* pts) const {
  return InputPoints::DecompressFromProto(stroke_, pts);
}

Status Stroke::GetMesh(const IMeshReader& mesh_reader, size_t lod_index,
                       Mesh* mesh) const {
  *mesh = Mesh();
  if (lod_index >= MeshCount()) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT, "bad index to GetMesh");
  }
  mesh->object_matrix = obj_to_world_;
  return mesh_reader.LodToMesh(stroke_.lod(lod_index), shader_type_,
                               stroke_.abgr(), mesh);
}

Status Stroke::GetCoverage(size_t lod_index, float* max_coverage) const {
  if (lod_index >= MeshCount()) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "bad index to GetCoverage");
  }
  const ink::proto::LOD& lod = stroke_.lod(lod_index);
  float coverage = lod.max_coverage();
  if (!BoundsCheckExInc(coverage, 0, 1)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT, "invalid max coverage $0",
                       coverage);
  }

  *max_coverage = coverage;
  return OkStatus();
}

// static
void Stroke::WriteToProto(ink::proto::Stroke* stroke_proto,
                          const Stroke& stroke) {
  *stroke_proto = stroke.stroke_;
}

// static
void Stroke::WriteToProto(ink::proto::Element* element_proto,
                          const Stroke& stroke) {
  element_proto->Clear();
  element_proto->set_minimum_serializer_version(kSerializerVersion);
  ink::util::WriteToProto(element_proto->mutable_stroke(), stroke);
}

size_t Stroke::MeshCount() const { return stroke_.lod_size(); }

}  // namespace ink
