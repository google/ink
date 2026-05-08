// Copyright 2026 Google LLC
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

#ifndef INK_STROKES_INTERNAL_STROKE_SUBTRACTION_H_
#define INK_STROKES_INTERNAL_STROKE_SUBTRACTION_H_

#include "absl/status/statusor.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/partitioned_mesh.h"

namespace ink::strokes_internal {

// Given two meshes and transforms to a common coordinate space, returns a
// mesh representing the boolean subtraction of `mesh_b` from `mesh_a`, obtained
// by removing all triangles of `mesh_a` that intersects `mesh_b`.
//
// The coordinate transformations are expected to be non-degenerate, and an
// error will be returned if not. The returned PartitionMesh is in the same
// coordinate space as `mesh_a`.
absl::StatusOr<PartitionedMesh> Subtract(const PartitionedMesh& mesh_a,
                                         const AffineTransform& transform_a,
                                         const PartitionedMesh& mesh_b,
                                         const AffineTransform& transform_b);

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_SUBTRACTION_H_
