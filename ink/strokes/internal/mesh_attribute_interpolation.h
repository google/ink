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

#ifndef INK_STROKES_INTERNAL_MESH_ATTRIBUTE_INTERPOLATION_H_
#define INK_STROKES_INTERNAL_MESH_ATTRIBUTE_INTERPOLATION_H_

#include <cstdint>

#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {

// The attributes of a single mesh vertex, mapped to coordinates in which they
// can be linearly interpolated.
//
// Position attributes and label attributes are omitted. Color shift
// attributes (`kColorShiftHcl`) are stored in a linearized form, and all other
// attributes are stored as-is.
//
// TODO(b/572902140): Consider merging these functions with `Lerp` and
// `BarycentricLerp` from stroke_vertex.h.
using InterpolationAttributes = absl::InlinedVector<SmallArray<float, 4>, 4>;

// Returns the attributes of `vertex` in `mesh`, mapped to coordinates in which
// they can be linearly interpolated.
InterpolationAttributes GetInterpolationAttributes(const Mesh& mesh,
                                                   uint32_t vertex);
InterpolationAttributes GetInterpolationAttributes(const MutableMesh& mesh,
                                                   uint32_t vertex);

// Sets the attributes of `vertex` in `mesh` by interpolating `sources` with the
// given `weights` (and mapping the result back from interpolation coordinates).
// Attributes that are not interpolated (position and labels) are left
// unchanged.
//
// `sources` must be non-empty and the same size as `weights`, and each element
// of `sources` must be consistent with the `mesh` format.
void SetInterpolatedAttributes(
    MutableMesh& mesh, uint32_t vertex,
    absl::Span<const InterpolationAttributes> sources,
    absl::Span<const float> weights);

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_MESH_ATTRIBUTE_INTERPOLATION_H_
