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

#include "ink/engine/scene/data/common/openctm_serializer.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "third_party/absl/memory/memory.h"
#include "third_party/openctm/files/lib/ctmlimits.h"
#include "third_party/openctm/files/lib/openctm.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/gl_managers/texture_info.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {

namespace {

const int kMaxVerticesPerMesh = 28000;       // 28 KiB
const int kMaxOpenCtmAllocation = 10000000;  // 10 MiB
const float kMaxDimension = 4096;
const int kMaxCtmStringReadLength = 1024;

// Returns maximum possible vertex scalar value for given mesh.
float GetScaleFactor(const OptimizedMesh& m) {
  return PackedVertList::GetMaxCoordinateForFormat(m.verts.GetFormat());
}

float GetScaleFactor(ShaderType shader_type) {
  return PackedVertList::GetMaxCoordinateForFormat(
      OptimizedMesh::VertexFormat(shader_type));
}

// RAII wrapper for a CTMcontext.
class CTMContextHolder {
 public:
  explicit CTMContextHolder(CTMenum mode) : context_(ctmNewContext(mode)) {}

  // Disallow copy and assign.
  CTMContextHolder(const CTMContextHolder&) = delete;
  CTMContextHolder& operator=(const CTMContextHolder&) = delete;

  ~CTMContextHolder() { ctmFreeContext(context_); }
  CTMcontext Get() const { return context_; }

 private:
  CTMcontext context_;
};

// This is supplied as a callback to ctmSaveCustom().
CTMuint WriteCTMToBuffer(const void* a_buf, CTMuint a_count,
                         void* a_user_data) {
  std::vector<char>* serialized_mesh =
      static_cast<std::vector<char>*>(a_user_data);
  const char* buf = static_cast<const char*>(a_buf);
  serialized_mesh->reserve(serialized_mesh->size() + a_count);
  for (size_t i = 0; i < a_count; i++) {
    serialized_mesh->push_back(buf[i]);
  }
  return a_count;
}

// Used by readCTMFromBuffer() below.
struct ReaderContext {
  ReaderContext(size_t size, const char* buf)
      : serialized_mesh(buf), serialized_mesh_size(size), offset(0) {}
  const char* serialized_mesh;
  size_t serialized_mesh_size;
  size_t offset;
};

// This is supplied as a callback to ctmLoadCustom().
CTMuint ReadCTMFromBuffer(void* output_buffer, CTMuint n_bytes_to_read,
                          void* a_user_data) {
  ReaderContext* reader_context = static_cast<ReaderContext*>(a_user_data);
  char* char_output_buffer = static_cast<char*>(output_buffer);
  size_t count =
      std::min(static_cast<size_t>(n_bytes_to_read),
               reader_context->serialized_mesh_size - reader_context->offset);
  memcpy(char_output_buffer,
         reader_context->serialized_mesh + reader_context->offset, count);
  reader_context->offset += count;
  return count;
}

Status S_WARN_UNUSED_RESULT CheckCtm(CTMcontext context) {
  auto err = ctmGetError(context);
  if (err != CTM_NONE) {
    return ErrorStatus(StatusCode::FAILED_PRECONDITION, "$0",
                       ctmErrorString(err));
  }
  return OkStatus();
}

// Returns true on success
Status S_WARN_UNUSED_RESULT DeserializeVerticesToMesh(CTMcontext context,
                                                      ShaderType shader_type,
                                                      uint32_t solid_agbr,
                                                      Mesh* mesh) {
  const CTMfloat* vertices = ctmGetFloatArray(context, CTM_VERTICES);
  if (vertices == nullptr) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT, "Missing vertices array");
  }

  // Check for vertex coloring.
  const CTMfloat* packed_rgba_array = nullptr;
  CTMenum colors_id = ctmGetNamedAttribMap(context, "Color");
  if (colors_id != CTM_NONE) {
    packed_rgba_array = ctmGetFloatArray(context, colors_id);
    if (packed_rgba_array == nullptr) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "Invalid Color attribute map");
    }
  } else if (mesh_serialization::IsVertexColored(shader_type)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Vertex colored but missing Color attribute map");
  }

  Vertex v;
  if (!packed_rgba_array) {
    // uintToVec4ABGR clamps, so v.color is safe
    v.color = UintToVec4ABGR(solid_agbr);
  }

  // Check for texture coords.
  const bool read_uv = (shader_type == TexturedVertShader);
  const CTMfloat* uv = nullptr;
  if (read_uv) {
    CTMenum uv_id = ctmGetNamedUVMap(context, "UV");
    if (uv_id == CTM_NONE) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                         "Textured but missing UV map");
    }
    uv = ctmGetFloatArray(context, uv_id);
    if (uv == nullptr) {
      return ErrorStatus(StatusCode::INVALID_ARGUMENT, "Invalid UV map");
    }
    const char* texture_uri_cstr =
        ctmGetUVMapString(context, uv_id, CTM_FILE_NAME);
    INK_RETURN_UNLESS(CheckCtm(context));
    std::string texture_uri(texture_uri_cstr);

    mesh->texture = absl::make_unique<TextureInfo>(texture_uri);
  }

  const float vertex_scale = GetScaleFactor(shader_type);
  const CTMuint vertex_count = ctmGetInteger(context, CTM_VERTEX_COUNT);
  INK_RETURN_UNLESS(CheckCtm(context));
  if (vertex_count > kMaxVerticesPerMesh) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "wanted $0 vertex count > max vertex count $1",
                       vertex_count, kMaxVerticesPerMesh);
  }
  mesh->verts.reserve(mesh->verts.size() + vertex_count);
  for (CTMuint j = 0; j < vertex_count; j++) {
    v.position.x = vertices[j * 3] * vertex_scale;
    v.position.y = vertices[j * 3 + 1] * vertex_scale;
    INK_RETURN_UNLESS(BoundsCheckIncEx(v.position, 0, kMaxDimension));

    if (packed_rgba_array) {
      v.color.r = packed_rgba_array[j * 4];
      v.color.g = packed_rgba_array[j * 4 + 1];
      v.color.b = packed_rgba_array[j * 4 + 2];
      v.color.a = packed_rgba_array[j * 4 + 3];
      INK_RETURN_UNLESS(BoundsCheckIncInc(v.color, 0, 1));
    }

    if (read_uv) {
      v.texture_coords.s = uv[j * 2];
      v.texture_coords.t = uv[j * 2 + 1];
      INK_RETURN_UNLESS(
          BoundsCheckIncInc(v.texture_coords, -kMaxDimension, kMaxDimension));
    }

    mesh->verts.push_back(v);
  }
  return OkStatus();
}

// Decodes indices into mesh arg. Returns true on success.
S_WARN_UNUSED_RESULT Status DeserializeIndicesToMesh(CTMcontext context,
                                                     Mesh* mesh) {
  const CTMuint* indices = ctmGetIntegerArray(context, CTM_INDICES);
  INK_RETURN_UNLESS(CheckCtm(context));
  if (indices == nullptr) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT, "Missing index array");
  }
  const CTMuint tri_count = ctmGetInteger(context, CTM_TRIANGLE_COUNT);
  INK_RETURN_UNLESS(CheckCtm(context));
  mesh->idx.assign(indices, indices + tri_count * 3);
  return OkStatus();
}

}  // namespace

S_WARN_UNUSED_RESULT Status OpenCtmReader::LodToMesh(const ink::proto::LOD& lod,
                                                     ShaderType shader_type,
                                                     uint32_t solid_abgr,
                                                     Mesh* mesh) const {
  CTMContextHolder context(CTM_IMPORT);
  // Protect from corrupt or malicious CTM meshes.
  CTMlimits limits;
  memset(&limits, 0, sizeof(CTMlimits));  // Thanks, msan!
  limits.maxAllocation = kMaxOpenCtmAllocation;
  limits.maxVertices = kMaxVerticesPerMesh;
  limits.maxTriangles = kMaxVerticesPerMesh;
  limits.maxStringLength = kMaxCtmStringReadLength;
  limits.forbidUVMaps = false;  // we use these
  limits.maxUVMaps = 1;
  limits.forbidAttributeMaps = false;  // we use these
  limits.maxAttributeMaps = 1;
  limits.forbidNormals = true;
  limits.forbidComment = true;
  ctmSetLimits(context.Get(), limits);
  ReaderContext reader_context(lod.ctm_blob().size(), lod.ctm_blob().c_str());
  ctmLoadCustom(context.Get(), &ReadCTMFromBuffer, &reader_context);
  INK_RETURN_UNLESS(CheckCtm(context.Get()));

  INK_RETURN_UNLESS(
      DeserializeVerticesToMesh(context.Get(), shader_type, solid_abgr, mesh));

  return DeserializeIndicesToMesh(context.Get(), mesh);
}

OpenCtmWriter::OpenCtmWriter(VertFormat format)
    : precision_bits_(PackedVertList::CalcRequiredPrecision(format)) {}

S_WARN_UNUSED_RESULT Status OpenCtmWriter::MeshToLod(
    const OptimizedMesh& mesh, ink::proto::LOD* lod) const {
  //
  // OpenCTM compression is more (memory) efficient and positions are more
  // precise if all vertices position coordinates lie in [0,1].  For formats
  // where vertex positions are bounded, rescale to [0,1].
  //
  const float vertex_max = GetScaleFactor(mesh);
  const size_t vertex_count = mesh.verts.size();
  const size_t index_count = mesh.IndexSize();
  const bool vertex_colored = mesh_serialization::IsVertexColored(mesh.type);

  Vertex unpacked_vertex;
  if (!vertex_colored) {
    unpacked_vertex.color = mesh.color;
  }

  CTMContextHolder context(CTM_EXPORT);
  ctmCompressionMethod(context.Get(), CTM_METHOD_MG2);

  ctmCompressionProvider(context.Get(), CTM_COMPRESSION_ZLIB);
  ctmCompressionLevel(context.Get(), 4);

  ctmVertexPrecision(context.Get(), 1.0f / (1 << precision_bits_));

  // Allocate vertex, index, color and texture arrays
  std::unique_ptr<CTMfloat[]> vertices(new CTMfloat[3 * vertex_count]);
  std::unique_ptr<CTMuint[]> indices(new CTMuint[index_count]);
  std::unique_ptr<CTMfloat[]> rgba;
  if (vertex_colored) {
    rgba = absl::make_unique<CTMfloat[]>(4 * vertex_count);
  }
  const bool serialize_uv = (mesh.type == TexturedVertShader);
  std::unique_ptr<CTMfloat[]> uv;
  if (serialize_uv) {
    uv = absl::make_unique<CTMfloat[]>(2 * vertex_count);
  }

  for (size_t j = 0, v = 0, r = 0, u = 0; j < vertex_count; j++) {
    mesh.verts.UnpackVertex(j, &unpacked_vertex);

    vertices[v++] = unpacked_vertex.position.x / vertex_max;
    vertices[v++] = unpacked_vertex.position.y / vertex_max;
    vertices[v++] = 0.0f;  // unused z

    if (vertex_colored) {
      rgba[r++] = unpacked_vertex.color.r;
      rgba[r++] = unpacked_vertex.color.g;
      rgba[r++] = unpacked_vertex.color.b;
      rgba[r++] = unpacked_vertex.color.a;
    }

    if (serialize_uv) {
      uv[u++] = unpacked_vertex.texture_coords.s;
      uv[u++] = unpacked_vertex.texture_coords.t;
    }
  }

  for (size_t j = 0; j < index_count; j++) {
    indices[j] = mesh.IndexAt(j);
  }

  ctmDefineMesh(context.Get(), vertices.get(), vertex_count, indices.get(),
                index_count / 3, nullptr);

  if (vertex_colored) {
    if (ctmAddAttribMap(context.Get(), rgba.get(), "Color") == CTM_NONE) {
      RUNTIME_ERROR("failed to add Color attribute map");
    }
  }
  if (serialize_uv) {
    ctmAddUVMap(context.Get(), uv.get(), "UV", mesh.texture->uri.c_str());
  }

  std::vector<char> serialized_mesh;

  // Reserve a conservative guess of 5 bytes per vertex.
  serialized_mesh.reserve(vertex_count * 5);
  ctmSaveCustom(context.Get(), &WriteCTMToBuffer, &serialized_mesh);
  lod->set_ctm_blob(serialized_mesh.data(), serialized_mesh.size());
  return OkStatus();
}
}  // namespace ink
