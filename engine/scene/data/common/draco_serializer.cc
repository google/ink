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

#include "ink/engine/scene/data/common/draco_serializer.h"

#include "third_party/draco/src/draco/compression/decode.h"
#include "third_party/draco/src/draco/compression/encode.h"
#include "third_party/draco/src/draco/mesh/mesh.h"
#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/type_ptr.hpp"
#include "ink/engine/colors/colors.h"

namespace ink {

static constexpr char kTextureMetadataHeaderKey[] = "name";
static constexpr char kTextureMetadataHeaderValue[] = "texture";
static constexpr char kTextureMetadataUriKey[] = "uri";
const int kMaxVerticesPerMesh = 1000000;  // \drevil{ONE MILLION VERTICES}

using draco::AttributeMetadata;
using draco::AttributeValueIndex;
using draco::DT_FLOAT32;
using draco::DT_UINT8;
using draco::GeometryAttribute;
using draco::PointAttribute;
using draco::PointIndex;

DracoWriter::DracoWriter(VertFormat format, int speed)
    : precision_bits_(PackedVertList::CalcRequiredPrecision(format)),
      speed_(speed) {}

Status DracoWriter::MeshToLod(const OptimizedMesh& ink_mesh,
                              ink::proto::LOD* lod) const {
  draco::Mesh draco_mesh;

  const size_t index_count = ink_mesh.idx.size();
  if (index_count % 3 != 0) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "can't encode mesh with index count $0", index_count);
  }
  draco_mesh.set_num_points(index_count);

  const size_t vertex_count = ink_mesh.verts.size();
  GeometryAttribute pos_att_descriptor;
  pos_att_descriptor.Init(GeometryAttribute::POSITION,  // attribute type
                          nullptr,                      // pre-existing buffer
                          2,                            // components per vertex
                          DT_FLOAT32,                   // draco data type
                          false,                        // normalized
                          sizeof(float) * 2,            // byte stride
                          0);                           // offset
  const int pos_att_id =
      draco_mesh.AddAttribute(pos_att_descriptor, false, vertex_count);
  draco::PointAttribute* const position_attribute =
      draco_mesh.attribute(pos_att_id);

  draco::PointAttribute* texture_attribute{nullptr};
  if (ink_mesh.type == TexturedVertShader) {
    if (!ink_mesh.texture || ink_mesh.texture->uri.empty()) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "textured shader with no texture on mesh");
    }

    GeometryAttribute ga;
    ga.Init(GeometryAttribute::TEX_COORD, nullptr, 2, DT_FLOAT32, false,
            sizeof(float) * 2, 0);
    int tex_att_id = draco_mesh.AddAttribute(ga, false, vertex_count);
    texture_attribute = draco_mesh.attribute(tex_att_id);

    auto texture_uri_metadata = absl::make_unique<AttributeMetadata>();
    texture_uri_metadata->AddEntryString(kTextureMetadataHeaderKey,
                                         kTextureMetadataHeaderValue);
    texture_uri_metadata->AddEntryString(kTextureMetadataUriKey,
                                         ink_mesh.texture->uri);
    draco_mesh.AddAttributeMetadata(tex_att_id,
                                    std::move(texture_uri_metadata));
  }

  draco::PointAttribute* color_attribute{nullptr};
  if (mesh_serialization::IsVertexColored(ink_mesh.type)) {
    GeometryAttribute ga;
    ga.Init(GeometryAttribute::COLOR, nullptr, 4 /* RGBA */, DT_UINT8, true,
            sizeof(uint8_t) * 4 /* RGBA */, 0);
    int color_attribute_id = draco_mesh.AddAttribute(ga, false, vertex_count);
    color_attribute = draco_mesh.attribute(color_attribute_id);
  }

  for (int i = 0; i < vertex_count; i++) {
    Vertex v;
    ink_mesh.verts.UnpackVertex(i, &v);
    const AttributeValueIndex vi(i);
    position_attribute->SetAttributeValue(vi, glm::value_ptr(v.position));
    if (texture_attribute) {
      texture_attribute->SetAttributeValue(vi,
                                           glm::value_ptr(v.texture_coords));
    }
    if (color_attribute) {
      std::array<uint8_t, 4> rgba{static_cast<uint8_t>(v.color.r * 255),
                                  static_cast<uint8_t>(v.color.g * 255),
                                  static_cast<uint8_t>(v.color.b * 255),
                                  static_cast<uint8_t>(v.color.a * 255)};
      color_attribute->SetAttributeValue(vi, &rgba[0]);
    }
  }

  const auto face_count = index_count / 3;

  PointIndex point_index(0);
  int ink_index = 0;
  for (int i = 0; i < face_count; i++) {
    draco::Mesh::Face draco_face;
    for (int c = 0; c < 3; ++c) {
      const auto entry_index = AttributeValueIndex(ink_mesh.idx[ink_index++]);
      position_attribute->SetPointMapEntry(point_index, entry_index);
      if (texture_attribute)
        texture_attribute->SetPointMapEntry(point_index, entry_index);
      if (color_attribute)
        color_attribute->SetPointMapEntry(point_index, entry_index);
      draco_face[c] = point_index++;
    }
    draco_mesh.AddFace(draco_face);
  }
  draco_mesh.DeduplicatePointIds();

  draco::EncoderBuffer buffer;
  draco::Encoder encoder;
  encoder.SetEncodingMethod(draco::MESH_EDGEBREAKER_ENCODING);
  encoder.SetSpeedOptions(speed_, speed_);
  std::array<float, 2> origin{0, 0};
  encoder.SetAttributeExplicitQuantization(
      GeometryAttribute::POSITION, precision_bits_, 2, &origin[0],
      PackedVertList::GetMaxCoordinateForFormat(ink_mesh.verts.GetFormat()));
  if (!encoder.EncodeMeshToBuffer(draco_mesh, &buffer).ok()) {
    return ErrorStatus(StatusCode::INTERNAL, "draco can't encode mesh");
  }
  lod->set_draco_blob(buffer.data(), buffer.size());
  return OkStatus();
}

Status DracoReader::LodToMesh(const ink::proto::LOD& lod,
                              ShaderType shader_type, uint32_t solid_abgr,
                              Mesh* out) const {
  draco::Mesh draco_mesh;

  draco::DecoderBuffer buffer;
  buffer.Init(lod.draco_blob().data(), lod.draco_blob().size());
  draco::Decoder decoder;
  auto status = decoder.DecodeBufferToGeometry(&buffer, &draco_mesh);
  if (!status.ok()) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT, "$0",
                       status.error_msg_string());
  }

  const PointAttribute* const pos_att =
      draco_mesh.GetNamedAttribute(GeometryAttribute::POSITION);
  if (!pos_att) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "No position attribute in the input mesh.");
  }
  const auto num_vertices = pos_att->size();
  if (num_vertices > kMaxVerticesPerMesh) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "vertex count of $0 > max vertex count $1", num_vertices,
                       kMaxVerticesPerMesh);
  }

  if (pos_att->num_components() != 2 ||
      pos_att->data_type() != draco::DT_FLOAT32) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Position attribute needs to contain 2 float32 components.");
  }

  const PointAttribute* const tex_att =
      draco_mesh.GetNamedAttribute(GeometryAttribute::TEX_COORD);
  if (shader_type == TexturedVertShader && !tex_att) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Expected textured vertices, but no texture attribute found.");
  }

  if (tex_att) {
    const auto* const mesh_metadata = draco_mesh.GetMetadata();
    if (!mesh_metadata) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "Expected textured mesh, but no mesh metadata found.");
    }
    const auto* texture_metadata =
        mesh_metadata->GetAttributeMetadataByStringEntry(
            kTextureMetadataHeaderKey, kTextureMetadataHeaderValue);
    if (!texture_metadata) {
      return ErrorStatus(
          StatusCode::INVALID_ARGUMENT,
          "Expected textured mesh, but no texture metadata found.");
    }

    std::string uri;
    if (!texture_metadata->GetEntryString(kTextureMetadataUriKey, &uri) ||
        uri.empty()) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "Expected textured mesh, but no texture uri found.");
    }
    out->texture = absl::make_unique<TextureInfo>(uri);
  }

  const PointAttribute* const color_att =
      draco_mesh.GetNamedAttribute(GeometryAttribute::COLOR);
  if (mesh_serialization::IsVertexColored(shader_type) && !color_att) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "Expected per-vertex colors, but no color attribute found.");
  }

  const auto num_faces = draco_mesh.num_faces();
  out->idx.clear();
  out->idx.reserve(num_faces * 3);
  for (draco::FaceIndex i(0); i < num_faces; ++i) {
    const auto face = draco_mesh.face(i);
    for (int j = 0; j < 3; j++) {
      auto vertex_index =
          static_cast<uint16_t>(pos_att->mapped_index(face[j]).value());
      if (vertex_index >= num_vertices) {
        return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                           "vertex index $0 >= vertex count $1", vertex_index,
                           num_vertices);
      }
      out->idx.emplace_back(vertex_index);
    }
  }

  Vertex v;
  v.color = UintToVec4ABGR(solid_abgr);
  for (AttributeValueIndex i(0); i < num_vertices; ++i) {
    if (!pos_att->ConvertValue<float, 2>(i, &v.position[0])) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT, "cannot read vertex $0",
                         i.value());
    }
    if (tex_att && !tex_att->ConvertValue<float, 2>(i, &v.texture_coords[0])) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "cannot read texture coordinate $0", i.value());
    }
    if (color_att) {
      std::array<uint8_t, 4> rgba{0, 0, 0, 0};
      if (!color_att->ConvertValue<uint8_t, 4>(i, &rgba[0])) {
        return ErrorStatus(StatusCode::INVALID_ARGUMENT, "cannot read color $0",
                           i.value());
      }
      v.color.r = static_cast<float>(rgba[0]) / 255.0f;
      v.color.g = static_cast<float>(rgba[1]) / 255.0f;
      v.color.b = static_cast<float>(rgba[2]) / 255.0f;
      v.color.a = static_cast<float>(rgba[3]) / 255.0f;
    }
    out->verts.push_back(v);
  }

  return OkStatus();
}
}  // namespace ink
