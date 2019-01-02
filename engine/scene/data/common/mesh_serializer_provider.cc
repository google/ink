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

#include "ink/engine/scene/data/common/mesh_serializer_provider.h"

#include "third_party/absl/base/call_once.h"
#include "third_party/absl/memory/memory.h"

#if MESH_COMPRESSION_DRACO
#include "ink/engine/scene/data/common/draco_serializer.h"
#endif
#if MESH_COMPRESSION_OPENCTM
#include "ink/engine/scene/data/common/openctm_serializer.h"
#endif

namespace ink {
namespace mesh {

namespace {
absl::once_flag log_once;
}  // namespace

std::unique_ptr<IMeshReader> ReaderFor(const Stroke& stroke) {
  if (stroke.Proto().lod_size() == 0) {
    SLOG(SLOG_WARNING, "No LOD in stroke; returning stub reader.");
    return absl::make_unique<StubMeshReader>();
  }
#if MESH_COMPRESSION_DRACO
  if (stroke.Proto().lod(0).has_draco_blob()) {
    return absl::make_unique<DracoReader>();
  }
#endif
#if MESH_COMPRESSION_OPENCTM
  if (stroke.Proto().lod(0).has_ctm_blob()) {
    return absl::make_unique<OpenCtmReader>();
  }
#endif
  // If the stroke was encoded with Draco or OpenCTM but we couldn't
  // deserialize the stroke due to compilation options, give a clear error.
  if (stroke.Proto().lod(0).has_draco_blob()) {
    SLOG(SLOG_ERROR, "Draco reader not available; returning stub reader.");
  }
  if (stroke.Proto().lod(0).has_ctm_blob()) {
    SLOG(SLOG_ERROR, "OpenCTM reader not available; returning stub reader.");
  }
  return absl::make_unique<StubMeshReader>();
}

std::unique_ptr<IMeshWriter> WriterFor(const OptimizedMesh& mesh) {
  absl::call_once(log_once, [&]() {
    SLOG(SLOG_INFO, "mesh compressor: $0", MeshCompressorName());
  });
  // Always write with Draco if available.
#if MESH_COMPRESSION_DRACO
  return absl::make_unique<DracoWriter>(mesh.verts.GetFormat());
#elif MESH_COMPRESSION_OPENCTM
  return absl::make_unique<OpenCtmWriter>(mesh.verts.GetFormat());
#else
  return absl::make_unique<StubMeshWriter>();
#endif
}

std::vector<MeshCompressorType> MeshCompressors() {
  std::vector<MeshCompressorType> result;
#if MESH_COMPRESSION_DRACO
  result.push_back(MeshCompressorType::DRACO);
#endif
#if MESH_COMPRESSION_OPENCTM
  result.push_back(MeshCompressorType::OPENCTM);
#endif
  return result;
}

std::string MeshCompressorName() {
  std::string result;
#if MESH_COMPRESSION_DRACO
  result.append("Draco");
#endif
#if MESH_COMPRESSION_OPENCTM
  if (!result.empty()) result += "/";
  result.append("OpenCTM");
#endif
  if (result.empty()) {
    result = "NoMeshCompression";
  }
  return result;
}

MeshCompressorType MeshCompressorFor(const proto::ElementBundle& bundle) {
  if (!bundle.has_element()) return MeshCompressorType::NONE;
  if (!bundle.element().has_stroke()) return MeshCompressorType::NONE;
  if (bundle.element().stroke().lod_size() == 0)
    return MeshCompressorType::NONE;
  if (bundle.element().stroke().lod(0).has_draco_blob())
    return MeshCompressorType::DRACO;
  if (bundle.element().stroke().lod(0).has_ctm_blob())
    return MeshCompressorType::OPENCTM;
  return MeshCompressorType::NONE;
}

}  // namespace mesh
}  // namespace ink
