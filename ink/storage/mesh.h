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

#ifndef INK_STORAGE_MESH_H_
#define INK_STORAGE_MESH_H_

#include "absl/status/statusor.h"
#include "ink/geometry/mesh.h"
#include "ink/storage/mesh_format.h"
#include "ink/storage/proto/mesh.pb.h"

namespace ink {

// Populates the `CodedMesh` by encoding the given Mesh.
//
// The `CodedMesh` need not be empty before calling this; this will effectively
// clear the `CodedMesh` first, but will reuse any existing allocations in the
// proto if possible.
void EncodeMesh(const Mesh& mesh, ink::proto::CodedMesh& coded_mesh);

// Populates the `CodedMesh` by encoding the given `Mesh`, omitting the mesh
// format. This can save space in contexts where the mesh format can be deduced
// by other means (e.g. within a `CodedModeledShape` proto).
void EncodeMeshOmittingFormat(const Mesh& mesh,
                              ink::proto::CodedMesh& coded_mesh);

// Decodes the `CodedMesh` into a Mesh. Returns an error if the proto is
// invalid.
absl::StatusOr<Mesh> DecodeMesh(const ink::proto::CodedMesh& coded_mesh);

// Decodes the `CodedMesh` into a `Mesh` using the provided `DecodedMeshFormat`
// (ignoring `CodedMesh.format`). Returns an error if the proto is invalid.
absl::StatusOr<Mesh> DecodeMeshUsingFormat(
    const DecodedMeshFormat& format, const ink::proto::CodedMesh& coded_mesh);

}  // namespace ink

#endif  // INK_STORAGE_MESH_H_
