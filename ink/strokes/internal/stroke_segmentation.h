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

#ifndef INK_STROKES_INTERNAL_STROKE_SEGMENTATION_H_
#define INK_STROKES_INTERNAL_STROKE_SEGMENTATION_H_

#include <vector>

#include "absl/status/statusor.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/partitioned_mesh.h"

namespace ink::strokes_internal {

// Returns a list of PartitionedMeshes, representing the connected components of
// `shape`. Here, connectedness is based on the geometry of the mesh embedded in
// R^2. In other words, two points in `shape` are connected if their transformed
// positions are within `tolerance` distance of each other.
//
// The order of the returned meshes is arbitrary. Each returned PartitionedMesh
// has the same number of render groups as the input. Every connected component
// is placed in the same render group as the group it originated from.
absl::StatusOr<std::vector<PartitionedMesh>> SegmentSpatially(
    const PartitionedMesh& shape, const AffineTransform& transform,
    float tolerance);

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_SEGMENTATION_H_
