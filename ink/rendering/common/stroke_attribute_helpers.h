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

#ifndef INK_RENDERING_COMMON_STROKE_ATTRIBUTE_HELPERS_H_
#define INK_RENDERING_COMMON_STROKE_ATTRIBUTE_HELPERS_H_

#include <optional>

#include "absl/status/statusor.h"
#include "ink/geometry/mesh_format.h"
#include "ink/strokes/internal/stroke_vertex.h"

namespace ink::rendering {

// Subsets of shader variable types for attributes that are used by Ink and
// available across platforms.
// LINT.IfChange(attribute_types)
enum class AttributeType {
  kFloat2 = 1,
  kFloat3 = 2,
  kUByte4 = 4,
};
// LINT.ThenChange(../../..//third_party/java/androidx/ink/rendering/main/android/canvas/internal/CanvasMeshRenderer.android.kt:shader_variable_types)

// Vertex attribute type and offset.
struct TypeAndByteOffset {
  AttributeType type;
  int offset;
};

struct StrokeAttributeTypesAndOffsets {
  TypeAndByteOffset position_and_opacity_shift;
  std::optional<TypeAndByteOffset> hsl_shift;
  TypeAndByteOffset side_derivative_and_label;
  TypeAndByteOffset forward_derivative_and_label;
  std::optional<TypeAndByteOffset> surface_uv_and_paint_animation_offset;
};

// Validates that the given `mesh_format` is supported and returns the shader
// variable types and byte offsets. `attribute_indices` is expected to hold
// precomputed values for the given `mesh_format`.
absl::StatusOr<StrokeAttributeTypesAndOffsets>
GetValidatedStrokeAttributeTypesAndOffsets(
    const MeshFormat& mesh_format,
    const strokes_internal::StrokeVertex::FormatAttributeIndices&
        attribute_indices);

StrokeAttributeTypesAndOffsets GetInProgressStrokeAttributeTypesAndOffsets(
    const MeshFormat& mesh_format,
    const strokes_internal::StrokeVertex::FormatAttributeIndices&
        attribute_indices);

}  // namespace ink::rendering

#endif  // INK_RENDERING_COMMON_STROKE_ATTRIBUTE_HELPERS_H_
