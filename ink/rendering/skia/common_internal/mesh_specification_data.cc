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
#include "absl/status/status.h"
#include "absl/status/status_macros.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush_paint.h"
#include "ink/geometry/mesh_format.h"
#include "ink/rendering/common/stroke_attribute_helpers.h"
#include "ink/rendering/skia/common_internal/sksl_common_shader_helper_functions.h"
#include "ink/rendering/skia/common_internal/sksl_fragment_shader_helper_functions.h"
#include "ink/rendering/skia/common_internal/sksl_vertex_shader_helper_functions.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/types/small_array.h"

namespace ink::skia_common_internal {
namespace {

using ::ink::rendering::GetValidatedStrokeAttributeTypesAndOffsets;
using ::ink::rendering::StrokeAttributeTypesAndOffsets;
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
constexpr absl::string_view kTextureMappingName = "uTextureMapping";
constexpr absl::string_view kTextureAnimationProgressName =
    "uTextureAnimationProgress";
constexpr absl::string_view kNumTextureAnimationFramesName =
    "uNumTextureAnimationFrames";
constexpr absl::string_view kNumTextureAnimationRowsName =
    "uNumTextureAnimationRows";
constexpr absl::string_view kNumTextureAnimationColumnsName =
    "uNumTextureAnimationColumns";

// Shared fragment shader used for both InProgressStroke and Stroke.
constexpr absl::string_view kFragmentMain = R"(
  float2 main(const Varyings varyings, out float4 color) {
    color =
      varyings.color * simulatedPixelCoverage(varyings.pixelsPerDimension,
                                              varyings.normalizedToEdgeLRFB,
                                              varyings.outsetPixelsLRFB);
    return varyings.textureCoords;
  })";

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
    case UniformId::kTextureMapping:
      return kTextureMappingName;
    case UniformId::kTextureAnimationProgress:
      return kTextureAnimationProgressName;
    case UniformId::kNumTextureAnimationFrames:
      return kNumTextureAnimationFramesName;
    case UniformId::kNumTextureAnimationRows:
      return kNumTextureAnimationRowsName;
    case UniformId::kNumTextureAnimationColumns:
      return kNumTextureAnimationColumnsName;
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
  static_assert(kUniformBrushColorName == "uBrushColor");
  static_assert(kObjectToCanvasLinearComponentName ==
                "uObjectToCanvasLinearComponent");
  static_assert(kTextureMappingName == "uTextureMapping");
  static_assert(kTextureAnimationProgressName == "uTextureAnimationProgress");
  static_assert(kNumTextureAnimationFramesName == "uNumTextureAnimationFrames");
  static_assert(kNumTextureAnimationRowsName == "uNumTextureAnimationRows");
  static_assert(kNumTextureAnimationColumnsName ==
                "uNumTextureAnimationColumns");
  // Do not use `layout(color)` for uBrushColor, as the color is being converted
  // into the shader color space manually rather than relying on the implicit
  // conversion of setColorUniform.
  constexpr absl::string_view kVertexMain = R"(
      uniform float4 uObjectToCanvasLinearComponent;
      uniform float4 uBrushColor;
      uniform int uTextureMapping;
      uniform float uTextureAnimationProgress;
      uniform int uNumTextureAnimationFrames;
      uniform int uNumTextureAnimationRows;
      uniform int uNumTextureAnimationColumns;

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

        if (uTextureMapping == 1) {
          varyings.textureCoords = calculateStampingTextureUv(
              unpackSurfaceUv(attributes.surfaceUvAndAnimationOffset.xy),
              unpackAnimationOffset(attributes.surfaceUvAndAnimationOffset.z),
              uTextureAnimationProgress,
              uNumTextureAnimationFrames,
              uNumTextureAnimationRows,
              uNumTextureAnimationColumns);
        } else {
          varyings.textureCoords = varyings.position;
        }

        return varyings;
      }
  )";
  static_assert(
      BrushPaint::TextureLayer{BrushPaint::StampingTexture{}}.index() == 1);

  // Translate from `MeshFormat` to `MeshSpecificationData` attributes. Where
  // applicable below, multiple `MeshFormat` attributes are combined into one
  // attribute for rendering.

  const MeshFormat kInProgressStrokeFormat = StrokeVertex::FullMeshFormat();
  constexpr StrokeVertex::FormatAttributeIndices kAttributeIndices =
      StrokeVertex::kFullFormatAttributeIndices;

  auto types_and_offsets =
      ink::rendering::GetInProgressStrokeAttributeTypesAndOffsets(
          kInProgressStrokeFormat, kAttributeIndices);

  constexpr int kRenderingAttributeCount = 5;
  SmallArray<Attribute, kMaxAttributes> rendering_attributes(
      kRenderingAttributeCount);

  rendering_attributes[0] = {
      .type = types_and_offsets.position_and_opacity_shift.type,
      .offset = types_and_offsets.position_and_opacity_shift.offset,
      .name = "positionAndOpacityShift"};

  rendering_attributes[1] = {.type = types_and_offsets.hsl_shift->type,
                             .offset = types_and_offsets.hsl_shift->offset,
                             .name = "hslShift"};

  rendering_attributes[2] = {
      .type = types_and_offsets.side_derivative_and_label.type,
      .offset = types_and_offsets.side_derivative_and_label.offset,
      .name = "sideDerivativeAndLabel"};

  rendering_attributes[3] = {
      .type = types_and_offsets.forward_derivative_and_label.type,
      .offset = types_and_offsets.forward_derivative_and_label.offset,
      .name = "forwardDerivativeAndLabel"};

  rendering_attributes[4] = {
      .type = types_and_offsets.surface_uv_and_animation_offset->type,
      .offset = types_and_offsets.surface_uv_and_animation_offset->offset,
      .name = "surfaceUvAndAnimationOffset"};

  return MeshSpecificationData{
      .attributes = rendering_attributes,
      .vertex_stride = kInProgressStrokeFormat.UnpackedVertexStride(),
      .varyings = {{.type = VaryingType::kFloat4, .name = "color"},
                   {.type = VaryingType::kFloat2, .name = "textureCoords"},
                   {.type = VaryingType::kFloat2, .name = "pixelsPerDimension"},
                   {.type = VaryingType::kFloat4,
                    .name = "normalizedToEdgeLRFB"},
                   {.type = VaryingType::kFloat4, .name = "outsetPixelsLRFB"}},
      .uniforms = {{.type = UniformType::kFloat4,
                    .id = UniformId::kObjectToCanvasLinearComponent},
                   {.type = UniformType::kFloat4, .id = UniformId::kBrushColor},
                   {.type = UniformType::kInt,
                    .id = UniformId::kTextureMapping},
                   {.type = UniformType::kFloat,
                    .id = UniformId::kTextureAnimationProgress},
                   {.type = UniformType::kInt,
                    .id = UniformId::kNumTextureAnimationFrames},
                   {.type = UniformType::kInt,
                    .id = UniformId::kNumTextureAnimationRows},
                   {.type = UniformType::kInt,
                    .id = UniformId::kNumTextureAnimationColumns}},
      .vertex_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLVertexShaderHelpers, kVertexMain),
      .fragment_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLFragmentShaderHelpers, kFragmentMain),
  };
}

absl::StatusOr<MeshSpecificationData> MeshSpecificationData::CreateForStroke(
    const MeshFormat& mesh_format) {
  StrokeVertex::FormatAttributeIndices attribute_indices =
      StrokeVertex::FindAttributeIndices(mesh_format);
  ABSL_ASSIGN_OR_RETURN(StrokeAttributeTypesAndOffsets types_and_offsets,
                        GetValidatedStrokeAttributeTypesAndOffsets(
                            mesh_format, attribute_indices));

  static_assert(kUniformBrushColorName == "uBrushColor");
  static_assert(kUniformPositionUnpackingTransformName ==
                "uPositionUnpackingTransform");
  static_assert(kObjectToCanvasLinearComponentName ==
                "uObjectToCanvasLinearComponent");
  static_assert(kUniformSideDerivativeUnpackingTransformName ==
                "uSideUnpackingTransform");
  static_assert(kUniformForwardDerivativeUnpackingTransformName ==
                "uForwardUnpackingTransform");
  static_assert(kTextureMappingName == "uTextureMapping");
  static_assert(kTextureAnimationProgressName == "uTextureAnimationProgress");
  static_assert(kNumTextureAnimationFramesName == "uNumTextureAnimationFrames");
  static_assert(kNumTextureAnimationRowsName == "uNumTextureAnimationRows");
  static_assert(kNumTextureAnimationColumnsName ==
                "uNumTextureAnimationColumns");
  // Do not use `layout(color)` for uBrushColor, as the color is being converted
  // into the shader color space manually rather than relying on the implicit
  // conversion of setColorUniform.
  constexpr absl::string_view kVertexMainStart = R"(
      uniform float4 uObjectToCanvasLinearComponent;
      uniform float4 uBrushColor;
      uniform float4 uPositionUnpackingTransform;
      uniform float4 uSideUnpackingTransform;
      uniform float4 uForwardUnpackingTransform;
      uniform int uTextureMapping;
      uniform float uTextureAnimationProgress;
      uniform int uNumTextureAnimationFrames;
      uniform int uNumTextureAnimationRows;
      uniform int uNumTextureAnimationColumns;

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

  // There are three cases for computing texture coordinates in the shader.
  //
  // Case 1: 12-bit surface U and V and 8-bit animation offset. This is used for
  // particle-based meshes to support (potentially-animated) "stamping" textured
  // particles.
  static_assert(
      BrushPaint::TextureLayer{BrushPaint::StampingTexture{}}.index() == 1);
  constexpr absl::string_view
      kVertexMainTextureUvWithSurfaceUvAndAnimationOffset = R"(
        if (uTextureMapping == 1) {
          varyings.textureCoords = calculateStampingTextureUv(
              unpackSurfaceUv(attributes.surfaceUvAndAnimationOffset.xyz),
              unpackAnimationOffset(attributes.surfaceUvAndAnimationOffset.w),
              uTextureAnimationProgress,
              uNumTextureAnimationFrames,
              uNumTextureAnimationRows,
              uNumTextureAnimationColumns);
        } else {
          varyings.textureCoords = varyings.position;
        }
  )";
  // Case 2: 12-bit surface U, 20-bit surface V, and no animation offset. This
  // is used for extruded (non-particle-based) meshes to support winding
  // textured extruded strokes.
  //
  // TODO: b/330511293 - Support this case.
  //
  // Case 3: No surface UV or animation offset attribute is available at all;
  // stamping/winding textures are not supported for this mesh.
  constexpr absl::string_view kVertexMainTextureUvWithoutSurfaceUv = R"(
        varyings.textureCoords = varyings.position;
  )";

  constexpr absl::string_view kVertexMainEnd = R"(
        return varyings;
      }
  )";

  absl::InlinedVector<Attribute, 5> mesh_specification_attributes = {
      {.type = types_and_offsets.position_and_opacity_shift.type,
       .offset = types_and_offsets.position_and_opacity_shift.offset,
       .name = "positionAndOpacityShift"},
      {.type = types_and_offsets.side_derivative_and_label.type,
       .offset = types_and_offsets.side_derivative_and_label.offset,
       .name = "sideDerivativeAndLabel"},
      {.type = types_and_offsets.forward_derivative_and_label.type,
       .offset = types_and_offsets.forward_derivative_and_label.offset,
       .name = "forwardDerivativeAndLabel"}};

  if (types_and_offsets.hsl_shift.has_value()) {
    mesh_specification_attributes.push_back(
        {.type = types_and_offsets.hsl_shift->type,
         .offset = types_and_offsets.hsl_shift->offset,
         .name = "hslShift"});
  }
  if (types_and_offsets.surface_uv_and_animation_offset.has_value()) {
    mesh_specification_attributes.push_back(
        {.type = types_and_offsets.surface_uv_and_animation_offset->type,
         .offset = types_and_offsets.surface_uv_and_animation_offset->offset,
         .name = "surfaceUvAndAnimationOffset"});
  }

  return MeshSpecificationData{
      .attributes = SmallArray<Attribute, kMaxAttributes>(
          absl::MakeSpan(mesh_specification_attributes)),
      .vertex_stride = mesh_format.PackedVertexStride(),
      .varyings = {{.type = VaryingType::kFloat4, .name = "color"},
                   {.type = VaryingType::kFloat2, .name = "pixelsPerDimension"},
                   {.type = VaryingType::kFloat4,
                    .name = "normalizedToEdgeLRFB"},
                   {.type = VaryingType::kFloat4, .name = "outsetPixelsLRFB"},
                   {.type = VaryingType::kFloat2, .name = "textureCoords"}},
      .uniforms =
          {{.type = UniformType::kFloat4,
            .id = UniformId::kObjectToCanvasLinearComponent},
           {.type = UniformType::kFloat4, .id = UniformId::kBrushColor},
           {.type = UniformType::kFloat4,
            .id = UniformId::kPositionUnpackingTransform,
            .unpacking_attribute_index = attribute_indices.position},
           {.type = UniformType::kFloat4,
            .id = UniformId::kSideDerivativeUnpackingTransform,
            .unpacking_attribute_index = attribute_indices.side_derivative},
           {.type = UniformType::kFloat4,
            .id = UniformId::kForwardDerivativeUnpackingTransform,
            .unpacking_attribute_index = attribute_indices.forward_derivative},
           {.type = UniformType::kInt, .id = UniformId::kTextureMapping},
           {.type = UniformType::kFloat,
            .id = UniformId::kTextureAnimationProgress},
           {.type = UniformType::kInt,
            .id = UniformId::kNumTextureAnimationFrames},
           {.type = UniformType::kInt,
            .id = UniformId::kNumTextureAnimationRows},
           {.type = UniformType::kInt,
            .id = UniformId::kNumTextureAnimationColumns}},
      .vertex_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLVertexShaderHelpers, kVertexMainStart,
          types_and_offsets.hsl_shift.has_value()
              ? kVertexMainColorWithHslShift
              : kVertexMainColorWithoutHslShift,
          types_and_offsets.surface_uv_and_animation_offset.has_value()
              // TODO: b/330511293 - If there's a surface UV, but no animation
              // offset, use `kVertexMainTextureUvWithSurfaceUvOnly` here.
              ? kVertexMainTextureUvWithSurfaceUvAndAnimationOffset
              : kVertexMainTextureUvWithoutSurfaceUv,
          kVertexMainEnd),
      .fragment_shader_source = absl::StrCat(
          kSkSLCommonShaderHelpers, kSkSLFragmentShaderHelpers, kFragmentMain)};
}

}  // namespace ink::skia_common_internal
