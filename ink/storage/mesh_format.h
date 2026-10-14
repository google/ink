// Copyright 2024 Google LLC
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

#ifndef INK_STORAGE_MESH_FORMAT_H_
#define INK_STORAGE_MESH_FORMAT_H_

#include <cstdint>
#include <vector>

#include "absl/status/statusor.h"
#include "ink/geometry/mesh_format.h"
#include "ink/storage/proto/mesh.pb.h"

namespace ink {

// Populates the given proto by encoding the given MeshFormat object.
//
// The proto need not be empty before calling this; this will effectively clear
// the proto first.
void EncodeMeshFormat(const MeshFormat& format,
                      proto::MeshFormat& format_proto);

// A decoded `MeshFormat` and the component offsets needed to decode a
// `CodedMesh`.
struct DecodedMeshFormat {
  MeshFormat format;

  // For each attribute in `format.Attributes()`, the index of its first
  // component in `CodedMesh.other_attribute_components` (unused for position).
  std::vector<uint32_t> attribute_offsets = {0};
};

// Decodes the proto into a `DecodedMeshFormat` with the given `IndexFormat`.
// Unrecognized non-position attributes are omitted from `format`. Returns an
// error if the proto is invalid.
absl::StatusOr<DecodedMeshFormat> DecodeMeshFormat(
    const proto::MeshFormat& format_proto,
    MeshFormat::IndexFormat index_format);

}  // namespace ink

#endif  // INK_STORAGE_MESH_FORMAT_H_
