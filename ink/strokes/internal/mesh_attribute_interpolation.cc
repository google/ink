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

#include "ink/strokes/internal/mesh_attribute_interpolation.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/types/numbers.h"
#include "ink/types/small_array.h"

namespace ink::strokes_internal {
namespace {

using ::ink::numbers::kPi;

// Maps an HCL shift from polar (hue, chroma) to Cartesian coordinates, so
// that linear interpolation matches the linear interpolation of shifted Oklab
// colors during rasterization.
//
// LINT.IfChange(hcl_shift_linear_space)
SmallArray<float, 4> HclShiftToLinearSpace(SmallArray<float, 4> val) {
  ABSL_DCHECK_GE(val.Size(), 2);
  float hue_shift = 2.0f * kPi * val[0], chroma_shift = val[1];
  val[0] = (chroma_shift + 1) * std::cos(hue_shift);
  val[1] = (chroma_shift + 1) * std::sin(hue_shift);
  return val;
}

SmallArray<float, 4> LinearSpaceToHclShift(SmallArray<float, 4> val) {
  ABSL_DCHECK_GE(val.Size(), 2);
  float dx = val[0], dy = val[1];
  val[0] = std::atan2(dy, dx) / (2.0f * kPi);
  val[1] = std::hypot(dx, dy) - 1.0f;
  return val;
}
// LINT.ThenChange(../../rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h:apply_hcl_and_opacity_shift)

// Helper function that extracts and linearizes the vertex attributes of the
// given vertex in `mesh`.
template <typename MeshType>
InterpolationAttributes GetInterpolationAttributesImpl(const MeshType& mesh,
                                                       uint32_t vertex) {
  InterpolationAttributes attributes(mesh.Format().Attributes().size());
  for (uint32_t i = 0; i < attributes.size(); ++i) {
    MeshFormat::AttributeId id = mesh.Format().Attributes()[i].id;

    // Position and anti-aliasing labels are not interpolated.
    if (id == MeshFormat::AttributeId::kPosition ||
        id == MeshFormat::AttributeId::kSideLabel ||
        id == MeshFormat::AttributeId::kForwardLabel) {
      continue;
    }

    // Color shift attributes need to be linearized for interpolation; all other
    // attributes get linearly interpolated.
    if (id == MeshFormat::AttributeId::kColorShiftHcl) {
      attributes[i] =
          HclShiftToLinearSpace(mesh.FloatVertexAttribute(vertex, i));
    } else {
      attributes[i] = mesh.FloatVertexAttribute(vertex, i);
    }
  }
  return attributes;
}

}  // namespace

InterpolationAttributes GetInterpolationAttributes(const Mesh& mesh,
                                                   uint32_t vertex) {
  return GetInterpolationAttributesImpl(mesh, vertex);
}

InterpolationAttributes GetInterpolationAttributes(const MutableMesh& mesh,
                                                   uint32_t vertex) {
  return GetInterpolationAttributesImpl(mesh, vertex);
}

void SetInterpolatedAttributes(
    MutableMesh& mesh, uint32_t vertex,
    absl::Span<const InterpolationAttributes> sources,
    absl::Span<const float> weights) {
  ABSL_DCHECK_EQ(sources.size(), weights.size());
  ABSL_DCHECK(!sources.empty());
  const MeshFormat& format = mesh.Format();
  for (uint32_t attr = 0; attr < format.Attributes().size(); ++attr) {
    MeshFormat::AttributeId id = format.Attributes()[attr].id;
    if (sources[0][attr].Size() == 0) continue;

    SmallArray<float, 4> interp_val(sources[0][attr].Size());
    for (size_t k = 0; k < sources.size(); ++k) {
      for (uint8_t c = 0; c < interp_val.Size(); ++c) {
        interp_val[c] += weights[k] * sources[k][attr][c];
      }
    }

    // Map the HCL shift back from linear space.
    if (id == MeshFormat::AttributeId::kColorShiftHcl) {
      interp_val = LinearSpaceToHclShift(interp_val);
    }
    mesh.SetFloatVertexAttribute(vertex, attr, interp_val);
  }
}

}  // namespace ink::strokes_internal
