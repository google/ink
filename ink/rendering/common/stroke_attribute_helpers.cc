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

#include "ink/rendering/common/stroke_attribute_helpers.h"

#include <optional>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "ink/geometry/mesh_format.h"
#include "ink/strokes/internal/stroke_vertex.h"

namespace ink::rendering {

namespace {

// Returns the supported `AttributeType` for the combined packed
// position-and-opacity attribute based on their `MeshFormat::AttributeType`s.
std::optional<AttributeType> FindTypeForPositionAndOpacityShift(
    MeshFormat::AttributeType position_type,
    MeshFormat::AttributeType opacity_type) {
  if (position_type == MeshFormat::AttributeType::kFloat2PackedInOneFloat &&
      opacity_type == MeshFormat::AttributeType::kFloat1Unpacked) {
    return AttributeType::kFloat2;
  }
  if (position_type ==
          MeshFormat::AttributeType::kFloat2PackedInThreeUnsignedBytes_XY12 &&
      opacity_type ==
          MeshFormat::AttributeType::kFloat1PackedInOneUnsignedByte) {
    return AttributeType::kUByte4;
  }
  return std::nullopt;
}

// Returns the supported `AttributeType` for the HSL shift attribute based on
// its `MeshFormat::AttributeType`.
std::optional<AttributeType> FindTypeForHslShift(
    MeshFormat::AttributeType hsl_shift_type) {
  if (hsl_shift_type == MeshFormat::AttributeType::kFloat3Unpacked) {
    return AttributeType::kFloat3;
  }
  if (hsl_shift_type ==
      MeshFormat::AttributeType::kFloat3PackedInFourUnsignedBytes_XYZ10) {
    return AttributeType::kUByte4;
  }
  return std::nullopt;
}

// Returns the supported `AttributeType` for either the "side" or "forward"
// derivative-and-label attribute based on their `MeshFormat::AttributeType`s.
std::optional<AttributeType> FindTypeForDerivativeAndLabel(
    MeshFormat::AttributeType derivative_type,
    MeshFormat::AttributeType label_type) {
  if (derivative_type == MeshFormat::AttributeType::kFloat2Unpacked &&
      label_type == MeshFormat::AttributeType::kFloat1Unpacked) {
    return AttributeType::kFloat3;
  }
  if (derivative_type ==
          MeshFormat::AttributeType::kFloat2PackedInThreeUnsignedBytes_XY12 &&
      label_type == MeshFormat::AttributeType::kFloat1PackedInOneUnsignedByte) {
    return AttributeType::kUByte4;
  }
  return std::nullopt;
}

// Returns the supported `AttributeType` for the surface UV attribute based on
// its `MeshFormat::AttributeType`.
std::optional<AttributeType> FindTypeForSurfaceUvAndPaintAnimationOffset(
    MeshFormat::AttributeType surface_uv_type,
    std::optional<MeshFormat::AttributeType> paint_animation_offset_type) {
  if (surface_uv_type == MeshFormat::AttributeType::kFloat2Unpacked &&
      paint_animation_offset_type ==
          MeshFormat::AttributeType::kFloat1Unpacked) {
    return AttributeType::kFloat3;
  }
  if (surface_uv_type ==
          MeshFormat::AttributeType::kFloat2PackedInThreeUnsignedBytes_XY12 &&
      paint_animation_offset_type ==
          MeshFormat::AttributeType::kFloat1PackedInOneUnsignedByte) {
    return AttributeType::kUByte4;
  }
  if (surface_uv_type ==
          MeshFormat::AttributeType::kFloat2PackedInFourUnsignedBytes_X12_Y20 &&
      paint_animation_offset_type == std::nullopt) {
    return AttributeType::kUByte4;
  }
  return std::nullopt;
}

}  // namespace

absl::StatusOr<StrokeAttributeTypesAndOffsets>
GetValidatedStrokeAttributeTypesAndOffsets(
    const MeshFormat& mesh_format,
    const strokes_internal::StrokeVertex::FormatAttributeIndices&
        attribute_indices) {
  ABSL_DCHECK_NE(attribute_indices.position, -1);

  if (attribute_indices.opacity_shift == -1 ||
      attribute_indices.side_derivative == -1 ||
      attribute_indices.side_label == -1 ||
      attribute_indices.forward_derivative == -1 ||
      attribute_indices.forward_label == -1) {
    return absl::InvalidArgumentError(
        absl::StrCat("Attributes with id `kOpacityShift`, `kSideDerivative`, "
                     "`kSideLabel`, `kForwardDerivative`, and `kForwardLabel` "
                     "are required. Got `mesh_format`: ",
                     mesh_format));
  }

  // For each Skia attribute used for strokes, check that the order of
  // `MeshFormat` attributes is compatible and find a supported `AttributeType`.

  StrokeAttributeTypesAndOffsets result;
  absl::Span<const MeshFormat::Attribute> attributes = mesh_format.Attributes();

  // --------------------------------------------------------------------------
  // Position + opacity-shift
  if (attribute_indices.position + 1 != attribute_indices.opacity_shift) {
    return absl::InvalidArgumentError(absl::StrCat(
        "The `kOpacityShift` attribute must be immediately after the "
        "`kPosition` attribute. Got `mesh_format`: ",
        mesh_format));
  }
  std::optional<AttributeType> position_and_opacity_type =
      FindTypeForPositionAndOpacityShift(
          attributes[attribute_indices.position].type,
          attributes[attribute_indices.opacity_shift].type);
  if (!position_and_opacity_type.has_value()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported type combination for `kPosition` and "
                     "`kOpacity` attributes. Got `mesh_format`: ",
                     mesh_format));
  }
  result.position_and_opacity_shift = {
      .type = *position_and_opacity_type,
      .offset = attributes[attribute_indices.position].packed_offset};

  // --------------------------------------------------------------------------
  // HSL color-shift
  if (attribute_indices.hsl_shift != -1) {
    std::optional<AttributeType> hsl_shift_type =
        FindTypeForHslShift(attributes[attribute_indices.hsl_shift].type);
    if (!hsl_shift_type.has_value()) {
      return absl::InvalidArgumentError(
          absl::StrCat("Unsupported type for `kColorShiftHsl` attribute. Got "
                       "`mesh_format`: ",
                       mesh_format));
    }
    result.hsl_shift = {
        .type = *hsl_shift_type,
        .offset = attributes[attribute_indices.hsl_shift].packed_offset};
  }

  // --------------------------------------------------------------------------
  // Side derivative + label
  if (attribute_indices.side_derivative + 1 != attribute_indices.side_label) {
    return absl::InvalidArgumentError(
        absl::StrCat("The `kSideLabel` attribute must be immediately after the "
                     "`kSideDerivative` attribute. Got `mesh_format`: ",
                     mesh_format));
  }
  std::optional<AttributeType> side_derivative_and_label_type =
      FindTypeForDerivativeAndLabel(
          attributes[attribute_indices.side_derivative].type,
          attributes[attribute_indices.side_label].type);
  if (!side_derivative_and_label_type.has_value()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported type combination for `kSideDerivative` and "
                     "`kSideLabel` attributes. Got `mesh_format`: ",
                     mesh_format));
  }
  result.side_derivative_and_label = {
      .type = *side_derivative_and_label_type,
      .offset = attributes[attribute_indices.side_derivative].packed_offset};

  // --------------------------------------------------------------------------
  // Forward derivative + label
  if (attribute_indices.forward_derivative + 1 !=
      attribute_indices.forward_label) {
    return absl::InvalidArgumentError(absl::StrCat(
        "The `kForwardLabel` attribute must be immediately after the "
        "`kForwardDerivative` attribute. Got `mesh_format`: ",
        mesh_format));
  }
  std::optional<AttributeType> forward_derivative_and_label_type =
      FindTypeForDerivativeAndLabel(
          attributes[attribute_indices.forward_derivative].type,
          attributes[attribute_indices.forward_label].type);
  if (!forward_derivative_and_label_type.has_value()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Unsupported type combination for `kForwardDerivative` and "
        "`kForwardLabel` attributes. Got `mesh_format`: ",
        mesh_format));
  }
  result.forward_derivative_and_label = {
      .type = *forward_derivative_and_label_type,
      .offset = attributes[attribute_indices.forward_derivative].packed_offset};

  // --------------------------------------------------------------------------
  // Surface UV + paint animation offset
  if (attribute_indices.surface_uv != -1) {
    std::optional<MeshFormat::AttributeType> paint_animation_offset_type;
    if (attribute_indices.paint_animation_offset != -1) {
      if (attribute_indices.surface_uv + 1 !=
          attribute_indices.paint_animation_offset) {
        return absl::InvalidArgumentError(absl::StrCat(
            "The `kPaintAnimationOffset` attribute must be immediately after "
            "the `kSurfaceUv` attribute. Got `mesh_format`: ",
            mesh_format));
      }
      paint_animation_offset_type =
          attributes[attribute_indices.paint_animation_offset].type;
    }
    std::optional<AttributeType> surface_uv_and_paint_animation_offset_type =
        FindTypeForSurfaceUvAndPaintAnimationOffset(
            attributes[attribute_indices.surface_uv].type,
            paint_animation_offset_type);
    if (!surface_uv_and_paint_animation_offset_type.has_value()) {
      return absl::InvalidArgumentError(absl::StrCat(
          "Unsupported type combination for `kSurfaceUv` and "
          "`kPaintAnimationOffset` attributes. Got `mesh_format`: ",
          mesh_format));
    }
    result.surface_uv_and_paint_animation_offset = {
        .type = *surface_uv_and_paint_animation_offset_type,
        .offset = attributes[attribute_indices.surface_uv].packed_offset};
  }

  return result;
}

StrokeAttributeTypesAndOffsets GetInProgressStrokeAttributeTypesAndOffsets(
    const MeshFormat& mesh_format,
    const strokes_internal::StrokeVertex::FormatAttributeIndices&
        attribute_indices) {
  absl::Span<const MeshFormat::Attribute> format_attributes =
      mesh_format.Attributes();

  StrokeAttributeTypesAndOffsets result;

  result.position_and_opacity_shift = {
      .type = AttributeType::kFloat3,
      .offset = format_attributes[attribute_indices.position].unpacked_offset};

  result.hsl_shift = {
      .type = AttributeType::kFloat3,
      .offset = format_attributes[attribute_indices.hsl_shift].unpacked_offset};

  result.side_derivative_and_label = {
      .type = AttributeType::kFloat3,
      .offset =
          format_attributes[attribute_indices.side_derivative].unpacked_offset};

  result.forward_derivative_and_label = {
      .type = AttributeType::kFloat3,
      .offset = format_attributes[attribute_indices.forward_derivative]
                    .unpacked_offset};

  result.surface_uv_and_paint_animation_offset = {
      .type = AttributeType::kFloat3,
      .offset =
          format_attributes[attribute_indices.surface_uv].unpacked_offset};

  return result;
}

}  // namespace ink::rendering
