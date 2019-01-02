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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_MESH_SERIALIZER_PROVIDER_H_
#define INK_ENGINE_SCENE_DATA_COMMON_MESH_SERIALIZER_PROVIDER_H_

#include <memory>
#include <string>

#include "ink/engine/scene/data/common/mesh_compressor_type.h"
#include "ink/engine/scene/data/common/mesh_serializer.h"
#include "ink/engine/scene/data/common/stroke.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
namespace mesh {

// Provides an enum indicating which mesh compressors this build of Ink is
// compiled to use.
std::vector<MeshCompressorType> MeshCompressors();

// Provides a human-readable name describing which mesh compressors this build
// of Ink is compiled to use.
std::string MeshCompressorName();

MeshCompressorType MeshCompressorFor(const proto::ElementBundle& bundle);

// Provide an IMeshReader that can deserialize the meshes in given Stroke.
std::unique_ptr<IMeshReader> ReaderFor(const Stroke& stroke);

// Provide an IMeshWriter that can serialize the given mesh.
std::unique_ptr<IMeshWriter> WriterFor(const OptimizedMesh& mesh);

}  // namespace mesh
}  // namespace ink

#endif  // INK_ENGINE_SCENE_DATA_COMMON_MESH_SERIALIZER_PROVIDER_H_
