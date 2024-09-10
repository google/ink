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

#include "ink/rendering/skia/common_internal/mesh_specification_data.h"

#include <optional>

#include "absl/container/inlined_vector.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/geometry/mesh_format.h"
#include "ink/rendering/skia/common_internal/sksl_common_shader_helper_functions.h"
#include "ink/rendering/skia/common_internal/sksl_fragment_shader_helper_functions.h"
#include "ink/rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/types/small_array.h"

namespace ink::skia_common_internal {
namespace {

using ::ink::strokes_internal::StrokeVertex;

// Uniform names for each `MeshSpecificationData::UniformId`.
constexpr absl::string_view kObjectToCanvasLinearComponentName =
    "uObjectToCanvasLinearComponent";
constexpr absl::string_view kUniformBrushColorName = "uBrushColor";
constexpr absl::string_view kUniformPositionUnpackingTransformName =
    "uPositionUnpackingTransform";
constexpr absl::string_view kUniformSideDerivativeUnpackingTransformName =
    "uSideUnpackingTransform";
constexpr absl::string_view kUniformForwardDerivativeUnpackingTransformName =
    "uForwardUnpackingTransform";

}  // namespace

absl::string_view MeshSpecificationData::GetUniformName(UniformId uniform_id) {
  switch (uniform_id) {
    case UniformId::kObjectToCanvasLinearComponent:
      return kObjectToCanvasLinearComponentName;
    case UniformId::kBrushColor:
      return kUniformBrushColorName;
    case UniformId::kPositionUnpackingTransform:
      return kUniformPositionUnpackingTransformName;
    case UniformId::kSideDerivativeUnpackingTransform:
      return kUniformSideDerivativeUnpackingTransformName;
    case UniformId::kForwardDerivativeUnpackingTransform:
      return kUniformForwardDerivativeUnpackingTransformName;
  }
  return "";
}

absl::StatusOr<MeshSpecificationData>
MeshSpecificationData::CreateForInProgressStroke(
    const MeshFormat& mesh_format) {
  if (mesh_format != StrokeVertex::FullMeshFormat()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Got a `mesh_format` not from an `InProgressStroke`: ", mesh_format));
  }
  return CreateForInProgressStroke();
}

MeshSpecificationData MeshSpecificationData::CreateForInProgressStroke() {
  // TODO: b/284117747 - Add support to the vertex and fragment shaders for
  // rendering `InProgressStroke` with winding texture coordinates.
  static_assert(kUniformBrushColorName == "uBrushColor");
  static_assert(kObjectToCanvasLinearComponentName ==
                "uObjectToCanvasLinearComponent");
  // Do not use `layout(color)` for uBrushColor, as the color is being converted
  // into the shader color space manually rather than relying on the implicit
  // conversion of setColorUniform.
  constexpr absl::string_view kVertexMain = R"(
      uniform float4 uObjectToCanvasLinearComponent;
      uniform float4 uBrushColor;

      Varyings main(const Attributes attributes) {
        Varyings varyings;
        varyings.position = attributes.positionAndOpacityShift.xy;

        varyings.position += calculateAntialiasingAndPositionOutset(
            attributes.sideDerivativeAndLabel,
            attributes.forwardDerivativeAndLabel,
            mat2FromFloat4ColumnMajor(uObjectToCanvasLinearComponent),
            varyings.pixelsPerDimension,
            varyings.normalizedToEdgeLRFB,
            varyings.outsetPixelsLRFB);

        varyings.color = applyHSLAndOpacityShift(
            attributes.hslShift, attributes.positionAndOpacityShift.z,
            uBrushColor);
        varyings.color.rgb *= varyings.color.a;

        return varyings;
      }
  )";

  constexpr absl::string_view kFragmentMain = R"(
      float2 main(const Varyings varyings, out float4 color) {
        color =
          varyings.color * simulatedPixelCoverage(varyings.pixelsPerDimension,
                                                  varyings.normalizedToEdgeLRFB,
                                                  varyings.outsetPixelsLRFB);
        return varyings.position;
      }
  )";

  // Translate from `MeshFormat` to `MeshSpecificationData` attributes. Where
  // applicable below, multiple `MeshFormat` attributes are combined into one
  // attribute for rendering.

  const MeshFormat kInProgressStrokeFormat = StrokeVertex::FullMeshFormat();
  absl::Span<const MeshFormat::Attribute> format_attributes =
      kInProgressStrokeFormat.Attributes();
  constexpr StrokeVertex::FormatAttributeIndices kAttributeIndices =
      StrokeVertex::kFullFormatAttributeIndices;

  constexpr int kRenderingAttributeCount = 4;
  SmallArray<Attribute, kMaxAttributes> rendering_attributes(
      kRenderingAttributeCount);

  // Position + opacity-shift
  static_assert(kAttributeIndices.position + 1 ==
                kAttributeIndices.opacity_shift);
  rendering_attributes[0] = {
      .type = AttributeType::kFloat3,
      .offset = format_attributes[kAttributeIndices.position].unpacked_offset,
      .name = "positionAndOpacityShift"};

  // HSL color-shift
  rendering_attributes[1] = {
      .type = AttributeType::kFloat3,
      .offset = format_attributes[kAttributeIndices.hsl_shift].unpacked_offset,
      .name = "hslShift"};

  // Side derivative + label
  static_assert(kAttributeIndices.side_derivative + 1 ==
                kAttributeIndices.side_label);
  rendering_attributes[2] = {
      .type = AttributeType::kFloat3,
      .offset =
          format_attributes[kAttributeIndices.side_derivative].unpacked_offset,
      .name = "sideDerivativeAndLabel"};

  // Forward derivative + label
  static_assert(kAttributeIndices.forward_derivative + 1 ==
                kAttributeIndices.forward_label);
  rendering_attributes[3] = {
      .type = AttributeType::kFloat3,
      .offset = format_attributes[kAttributeIndices.forward_derivative]
                    .unpacked_offset,
      .name = "forwardDerivativeAndLabel"};

  return MeshSpecificationData{
      .attributes = rendering_attributes,
      .vertex_stride = kInProgressStrokeFormat.UnpackedVertexStride(),
      .varyings = {{.type = VaryingType::kFloat4, .name = "color"},
                   {.type = VaryingType::kFloat2, .name = "pixelsPerDimension"},
                   {.type = VaryingType::kFloat4,
                    .name = "normalizedToEdgeLRFB"},
                   {.type = VaryingType::kFloat4, .name = "outsetPixelsLRFB"}},
      .uniforms = {{.type = UniformType::kFloat4,
                    .id = UniformId::kObjectToCanvasLinearComponent},
                   {.type = UniformType::kFloat4,
                    .id = UniformId::kBrushColor}},
      .vertex_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLVertexShaderHelpers, kVertexMain),
      .fragment_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLFragmentShaderHelpers, kFragmentMain),
  };
}

namespace {

// Returns the supported `AttributeType` for the combined packed
// position-and-opacity attribute based on their `MeshFormat::AttributeType`s.
std::optional<MeshSpecificationData::AttributeType>
FindTypeForPositionAndOpacityShift(MeshFormat::AttributeType position_type,
                                   MeshFormat::AttributeType opacity_type) {
  if (position_type == MeshFormat::AttributeType::kFloat2PackedIn1Float &&
      opacity_type == MeshFormat::AttributeType::kFloat1Unpacked) {
    return MeshSpecificationData::AttributeType::kFloat2;
  }
  if (position_type ==
          MeshFormat::AttributeType::kFloat2PackedIn3UnsignedBytes_XY12 &&
      opacity_type == MeshFormat::AttributeType::kFloat1PackedIn1UnsignedByte) {
    return MeshSpecificationData::AttributeType::kUByte4;
  }
  return std::nullopt;
}

// Returns the supported `AttributeType` for the HSL shift attribute based on
// its `MeshFormat::AttributeType`.
std::optional<MeshSpecificationData::AttributeType> FindTypeForHslShift(
    MeshFormat::AttributeType hsl_shift_type) {
  if (hsl_shift_type == MeshFormat::AttributeType::kFloat3Unpacked) {
    return MeshSpecificationData::AttributeType::kFloat3;
  }
  if (hsl_shift_type ==
      MeshFormat::AttributeType::kFloat3PackedIn4UnsignedBytes_XYZ10) {
    return MeshSpecificationData::AttributeType::kUByte4;
  }
  return std::nullopt;
}

// Returns the supported `AttributeType` for either the "side" or "forward"
// derivative-and-label attribute based on their `MeshFormat::AttributeType`s.
std::optional<MeshSpecificationData::AttributeType>
FindTypeForDerivativeAndLabel(MeshFormat::AttributeType derivative_type,
                              MeshFormat::AttributeType label_type) {
  if (derivative_type == MeshFormat::AttributeType::kFloat2Unpacked &&
      label_type == MeshFormat::AttributeType::kFloat1Unpacked) {
    return MeshSpecificationData::AttributeType::kFloat3;
  }
  if (derivative_type ==
          MeshFormat::AttributeType::kFloat2PackedIn3UnsignedBytes_XY12 &&
      label_type == MeshFormat::AttributeType::kFloat1PackedIn1UnsignedByte) {
    return MeshSpecificationData::AttributeType::kUByte4;
  }
  return std::nullopt;
}

// Vertex attribute type and offset.
struct TypeAndByteOffset {
  MeshSpecificationData::AttributeType type;
  int offset;
};

struct SkiaStrokeAttributeTypesAndOffsets {
  TypeAndByteOffset position_and_opacity_shift;
  std::optional<TypeAndByteOffset> hsl_shift;
  TypeAndByteOffset side_derivative_and_label;
  TypeAndByteOffset forward_derivative_and_label;
};

// Validates that the given `mesh_format` is supported and returns the shader
// variable types and byte offsets. `attribute_indices` is expected to hold
// precomputed values for the given `mesh_format`.
absl::StatusOr<SkiaStrokeAttributeTypesAndOffsets>
GetValidatedStrokeAttributeTypesAndOffsets(
    const MeshFormat& mesh_format,
    const StrokeVertex::FormatAttributeIndices& attribute_indices) {
  // `MeshFormat` creation requires a position attribute:
  DCHECK_NE(attribute_indices.position, -1);

  // Opacity-shift and anti-aliasing attributes are currently always required.
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

  SkiaStrokeAttributeTypesAndOffsets result;
  absl::Span<const MeshFormat::Attribute> attributes = mesh_format.Attributes();

  // --------------------------------------------------------------------------
  // Position + opacity-shift
  if (attribute_indices.position + 1 != attribute_indices.opacity_shift) {
    return absl::InvalidArgumentError(absl::StrCat(
        "The `kOpacityShift` attribute must be immediately after the "
        "`kPosition` attribute. Got `mesh_format`: ",
        mesh_format));
  }
  auto position_and_opacity_type = FindTypeForPositionAndOpacityShift(
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
    auto hsl_shift_type =
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
  auto derivative_and_label_type = FindTypeForDerivativeAndLabel(
      attributes[attribute_indices.side_derivative].type,
      attributes[attribute_indices.side_label].type);
  if (!derivative_and_label_type.has_value()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported type combination for `kSideDerivative` and "
                     "`kSideLabel` attributes. Got `mesh_format`: ",
                     mesh_format));
  }
  result.side_derivative_and_label = {
      .type = *derivative_and_label_type,
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
  derivative_and_label_type = FindTypeForDerivativeAndLabel(
      attributes[attribute_indices.forward_derivative].type,
      attributes[attribute_indices.forward_label].type);
  if (!derivative_and_label_type.has_value()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unsupported type combination for `kForwardDerivative` "
                     "and `kForwardLabel` attributes. Got `mesh_format`: ",
                     mesh_format));
  }
  result.forward_derivative_and_label = {
      .type = *derivative_and_label_type,
      .offset = attributes[attribute_indices.forward_derivative].packed_offset};

  return result;
}

}  // namespace

absl::StatusOr<MeshSpecificationData> MeshSpecificationData::CreateForStroke(
    const MeshFormat& mesh_format) {
  StrokeVertex::FormatAttributeIndices attribute_indices =
      StrokeVertex::FindAttributeIndices(mesh_format);
  auto types_and_offsets = GetValidatedStrokeAttributeTypesAndOffsets(
      mesh_format, attribute_indices);
  if (!types_and_offsets.ok()) return types_and_offsets.status();

  // TODO: b/284117747 - Add support to the vertex and fragment shaders for
  // rendering `Stroke` with winding texture coordinates.
  static_assert(kUniformBrushColorName == "uBrushColor");
  static_assert(kUniformPositionUnpackingTransformName ==
                "uPositionUnpackingTransform");
  static_assert(kObjectToCanvasLinearComponentName ==
                "uObjectToCanvasLinearComponent");
  static_assert(kUniformSideDerivativeUnpackingTransformName ==
                "uSideUnpackingTransform");
  static_assert(kUniformForwardDerivativeUnpackingTransformName ==
                "uForwardUnpackingTransform");
  // Do not use `layout(color)` for uBrushColor, as the color is being converted
  // into the shader color space manually rather than relying on the implicit
  // conversion of setColorUniform.
  constexpr absl::string_view kVertexMainStart = R"(
      uniform float4 uObjectToCanvasLinearComponent;
      uniform float4 uBrushColor;
      uniform float4 uPositionUnpackingTransform;
      uniform float4 uSideUnpackingTransform;
      uniform float4 uForwardUnpackingTransform;

      Varyings main(const Attributes attributes) {
        Varyings varyings;

        float3 positionAndOpacityShift = unpackPositionAndOpacityShift(
            uPositionUnpackingTransform, attributes.positionAndOpacityShift);
        varyings.position = positionAndOpacityShift.xy;

        varyings.position += calculateAntialiasingAndPositionOutset(
            unpackDerivativeAndLabel(uSideUnpackingTransform,
                                     attributes.sideDerivativeAndLabel),
            unpackDerivativeAndLabel(uForwardUnpackingTransform,
                                     attributes.forwardDerivativeAndLabel),
            mat2FromFloat4ColumnMajor(uObjectToCanvasLinearComponent),
            varyings.pixelsPerDimension,
            varyings.normalizedToEdgeLRFB,
            varyings.outsetPixelsLRFB);
  )";
  constexpr absl::string_view kVertexMainColorWithHslShift = R"(
        varyings.color =
            applyHSLAndOpacityShift(unpackHSLColorShift(attributes.hslShift),
                                    positionAndOpacityShift.z, uBrushColor);
        varyings.color.rgb *= varyings.color.a;
  )";
  constexpr absl::string_view kVertexMainColorWithoutHslShift = R"(
        float a = applyOpacityShift(positionAndOpacityShift.z, uBrushColor.a);
        varyings.color = float4(uBrushColor.rgb * a, a);
  )";
  constexpr absl::string_view kVertexMainEnd = R"(
        return varyings;
      }
  )";

  constexpr absl::string_view kFragmentMain = R"(
      float2 main(const Varyings varyings, out float4 color) {
        color =
          varyings.color * simulatedPixelCoverage(varyings.pixelsPerDimension,
                                                  varyings.normalizedToEdgeLRFB,
                                                  varyings.outsetPixelsLRFB);
        return varyings.position;
      }
  )";

  absl::InlinedVector<Attribute, 4> mesh_specification_attributes = {
      {.type = types_and_offsets->position_and_opacity_shift.type,
       .offset = types_and_offsets->position_and_opacity_shift.offset,
       .name = "positionAndOpacityShift"},
      {.type = types_and_offsets->side_derivative_and_label.type,
       .offset = types_and_offsets->side_derivative_and_label.offset,
       .name = "sideDerivativeAndLabel"},
      {.type = types_and_offsets->forward_derivative_and_label.type,
       .offset = types_and_offsets->forward_derivative_and_label.offset,
       .name = "forwardDerivativeAndLabel"}};

  if (types_and_offsets->hsl_shift.has_value()) {
    mesh_specification_attributes.push_back(
        {.type = types_and_offsets->hsl_shift->type,
         .offset = types_and_offsets->hsl_shift->offset,
         .name = "hslShift"});
  }

  return MeshSpecificationData{
      .attributes = SmallArray<Attribute, kMaxAttributes>(
          absl::MakeSpan(mesh_specification_attributes)),
      .vertex_stride = mesh_format.PackedVertexStride(),
      .varyings = {{.type = VaryingType::kFloat4, .name = "color"},
                   {.type = VaryingType::kFloat2, .name = "pixelsPerDimension"},
                   {.type = VaryingType::kFloat4,
                    .name = "normalizedToEdgeLRFB"},
                   {.type = VaryingType::kFloat4, .name = "outsetPixelsLRFB"}},
      .uniforms = {{.type = UniformType::kFloat4,
                    .id = UniformId::kObjectToCanvasLinearComponent},
                   {.type = UniformType::kFloat4, .id = UniformId::kBrushColor},
                   {.type = UniformType::kFloat4,
                    .id = UniformId::kPositionUnpackingTransform,
                    .unpacking_attribute_index = attribute_indices.position},
                   {.type = UniformType::kFloat4,
                    .id = UniformId::kSideDerivativeUnpackingTransform,
                    .unpacking_attribute_index =
                        attribute_indices.side_derivative},
                   {.type = UniformType::kFloat4,
                    .id = UniformId::kForwardDerivativeUnpackingTransform,
                    .unpacking_attribute_index =
                        attribute_indices.forward_derivative}},
      .vertex_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLVertexShaderHelpers, kVertexMainStart,
          types_and_offsets->hsl_shift.has_value()
              ? kVertexMainColorWithHslShift
              : kVertexMainColorWithoutHslShift,
          kVertexMainEnd),
      .fragment_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLFragmentShaderHelpers, kFragmentMain)};
}

}  // namespace ink::skia_common_internal
