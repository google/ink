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

#ifndef INK_STROKES_INTERNAL_STROKE_ANTI_ALIASING_H_
#define INK_STROKES_INTERNAL_STROKE_ANTI_ALIASING_H_

#include <cstdint>
#include <vector>

#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"

namespace ink::strokes_internal {

// This file provides some functions for computing stroke anti-aliasing
// attributes (outlines, boundary labels, derivatives, and margins) on meshes
// that are either fully or partially (e.g. after stroke subtraction) missing
// them.
// TODO(b/572902140): Merge shared functionality between this library and
// `DerivativeCalculator`.

// Returns true if `format` has the attributes required for anti-aliasing.
bool HasAntiAliasingAttributes(const MeshFormat& format);

// Computes and returns the boundary loops of `mesh` (as clockwise oriented
// lists of vertex indices), and computes and sets the boundary labels of
// vertices based on the computed boundary outline (subdividing interior edges
// if needed to avoid mislabeled interior edges). If existing side derivatives
// are present in `mesh`, they are used to define the local coordinate frame for
// boundary labeling; otherwise, a default frame is used. The label computation
// is omitted if `mesh` does not have label and derivative attributes in its
// format.
//
// The parameter `vertex_mask` specifies the vertices for which to recompute
// labels, and requires that `vertex_mask.size() == mesh.VertexCount()` or
// `vertex_mask.empty()`. Labels are recomputed for all boundary vertices
// if `vertex_mask.empty()`.
std::vector<std::vector<uint32_t>> ComputeOutlinesAndLabels(
    MutableMesh& mesh, const std::vector<bool>& vertex_mask = {});

// Computes and sets the derivatives and margins of vertices based on the
// existing labels in `mesh`. This function does nothing if `mesh` does not have
// label and derivative attributes in its format.
//
// The parameter `vertex_mask` specifies the vertices for which to recompute
// derivatives and margins, and requires that
// `vertex_mask.size() == mesh.VertexCount()` or `vertex_mask.empty()`.
// Derivatives and margins are recomputed for all vertices if
// `vertex_mask.empty()`.
void ComputeDerivativesAndMargins(MutableMesh& mesh,
                                  const std::vector<bool>& vertex_mask = {});

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_ANTI_ALIASING_H_
