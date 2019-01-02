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

#include "ink/engine/scene/data/common/mesh_compression_converter.h"

#include "ink/engine/scene/data/common/draco_serializer.h"
#include "ink/engine/scene/data/common/mesh_compressor_type.h"
#include "ink/engine/scene/data/common/mesh_serializer_provider.h"
#include "ink/engine/scene/data/common/openctm_serializer.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {
namespace mesh {

namespace {

template <typename Reader, typename Writer>
Status Convert(const proto::ElementBundle& in, proto::ElementBundle* out) {
  out->Clear();
  if (MeshCompressorFor(in) == MeshCompressorType::NONE) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Cannot convert element without compressed mesh.");
  }
  if (MeshCompressorFor(in) == Writer::SupportedMeshCompressor()) {
    *out = in;
    return OkStatus();
  }
  Stroke stroke;
  if (!Stroke::ReadFromProto(in, &stroke)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Could not deserialize given proto.");
  }
  Reader reader;
  Mesh mesh;
  INK_RETURN_UNLESS(stroke.GetMesh(reader, 0, &mesh));
  *out = in;
  out->mutable_element()->mutable_stroke()->clear_lod();
  OptimizedMesh optmesh(stroke.shader_type(), mesh);
  Writer writer(optmesh.verts.GetFormat());
  INK_RETURN_UNLESS(writer.MeshToLod(
      optmesh, out->mutable_element()->mutable_stroke()->add_lod()));
  return OkStatus();
}

}  // namespace

Status ToDraco(const proto::ElementBundle& in, proto::ElementBundle* out) {
  return Convert<OpenCtmReader, DracoWriter>(in, out);
}

Status ToOpenCtm(const proto::ElementBundle& in, proto::ElementBundle* out) {
  return Convert<DracoReader, OpenCtmWriter>(in, out);
}
}  // namespace mesh
}  // namespace ink
