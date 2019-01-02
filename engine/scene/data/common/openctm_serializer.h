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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_OPENCTM_SERIALIZER_H_
#define INK_ENGINE_SCENE_DATA_COMMON_OPENCTM_SERIALIZER_H_

#include <cstdint>

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/scene/data/common/mesh_serializer.h"
#include "ink/engine/util/security.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

class OpenCtmWriter : public IMeshWriter {
 public:
  explicit OpenCtmWriter(VertFormat format);

  static mesh::MeshCompressorType SupportedMeshCompressor() {
    return mesh::MeshCompressorType::OPENCTM;
  }

  S_WARN_UNUSED_RESULT Status MeshToLod(const OptimizedMesh& mesh,
                                        ink::proto::LOD* lod) const override;

 private:
  uint32_t precision_bits_;
};

class OpenCtmReader : public IMeshReader {
 public:
  static mesh::MeshCompressorType SupportedMeshCompressor() {
    return mesh::MeshCompressorType::OPENCTM;
  }

  S_WARN_UNUSED_RESULT Status LodToMesh(const ink::proto::LOD& lod,
                                        ShaderType shader_type,
                                        uint32_t solid_abgr,
                                        Mesh* mesh) const override;
};
}  // namespace ink

#endif  // INK_ENGINE_SCENE_DATA_COMMON_OPENCTM_SERIALIZER_H_
