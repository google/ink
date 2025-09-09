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

#include "ink/rendering/skia/native/internal/mesh_uniform_data.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>

#include "absl/functional/function_ref.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/types/span.h"
#include "ink/brush/brush_paint.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_packing_types.h"
#include "ink/rendering/skia/common_internal/mesh_specification_data.h"
#include "include/core/SkData.h"
#include "include/core/SkMesh.h"
#include "include/core/SkRefCnt.h"

namespace ink::skia_native_internal {
namespace {

using ::ink::skia_common_internal::MeshSpecificationData;

SkMeshSpecification::Uniform::Type ExpectedSkiaUniformType(
    MeshSpecificationData::UniformId uniform_id) {
  switch (uniform_id) {
    case MeshSpecificationData::UniformId::kObjectToCanvasLinearComponent:
    case MeshSpecificationData::UniformId::kBrushColor:
    case MeshSpecificationData::UniformId::kPositionUnpackingTransform:
    case MeshSpecificationData::UniformId::kSideDerivativeUnpackingTransform:
    case MeshSpecificationData::UniformId::kForwardDerivativeUnpackingTransform:
      return SkMeshSpecification::Uniform::Type::kFloat4;
    case MeshSpecificationData::UniformId::kTextureMapping:
      return SkMeshSpecification::Uniform::Type::kInt;
    case MeshSpecificationData::UniformId::kTextureAnimationProgress:
      return SkMeshSpecification::Uniform::Type::kFloat;
    case MeshSpecificationData::UniformId::kNumTextureAnimationFrames:
      return SkMeshSpecification::Uniform::Type::kInt;
    case MeshSpecificationData::UniformId::kNumTextureAnimationRows:
      return SkMeshSpecification::Uniform::Type::kInt;
    case MeshSpecificationData::UniformId::kNumTextureAnimationColumns:
      return SkMeshSpecification::Uniform::Type::kInt;
  }
  ABSL_LOG(FATAL) << "Got `uniform_id` with non-enumerator value: "
                  << static_cast<int>(uniform_id);
}

int16_t FindUniformOffset(const SkMeshSpecification& spec,
                          MeshSpecificationData::UniformId uniform_id) {
  const SkMeshSpecification::Uniform* uniform =
      spec.findUniform(MeshSpecificationData::GetUniformName(uniform_id));
  if (uniform == nullptr ||
      uniform->type != ExpectedSkiaUniformType(uniform_id)) {
    return -1;
  }
  return uniform->offset;
}

std::optional<MeshSpecificationData::UniformId> FindUnpackingTransformUniformId(
    MeshFormat::AttributeId attribute_id) {
  switch (attribute_id) {
    case MeshFormat::AttributeId::kPosition:
      return MeshSpecificationData::UniformId::kPositionUnpackingTransform;
    case MeshFormat::AttributeId::kSideDerivative:
      return MeshSpecificationData::UniformId::
          kSideDerivativeUnpackingTransform;
    case MeshFormat::AttributeId::kForwardDerivative:
      return MeshSpecificationData::UniformId::
          kForwardDerivativeUnpackingTransform;
    default:
      return std::nullopt;
  }
}

}  // namespace

MeshUniformData::MeshUniformData() : data_(SkData::MakeEmpty()) {}

MeshUniformData::MeshUniformData(const SkMeshSpecification& spec)
    : data_(SkData::MakeUninitialized(spec.uniformSize())),
      object_to_canvas_linear_component_offset_(FindUniformOffset(
          spec,
          MeshSpecificationData::UniformId::kObjectToCanvasLinearComponent)),
      brush_color_offset_(FindUniformOffset(
          spec, MeshSpecificationData::UniformId::kBrushColor)),
      texture_mapping_offset_(FindUniformOffset(
          spec, MeshSpecificationData::UniformId::kTextureMapping)),
      texture_animation_progress_offset_(FindUniformOffset(
          spec, MeshSpecificationData::UniformId::kTextureAnimationProgress)),
      num_texture_animation_frames_offset_(FindUniformOffset(
          spec, MeshSpecificationData::UniformId::kNumTextureAnimationFrames)),
      num_texture_animation_rows_offset_(FindUniformOffset(
          spec, MeshSpecificationData::UniformId::kNumTextureAnimationRows)),
      num_texture_animation_columns_offset_(FindUniformOffset(
          spec,
          MeshSpecificationData::UniformId::kNumTextureAnimationColumns)) {}

namespace {

template <typename T, size_t N>
void SetUniformIfPresent(std::byte* writable_data, int16_t uniform_byte_offset,
                         const std::array<T, N>& array) {
  if (uniform_byte_offset < 0) return;
  std::memcpy(writable_data + uniform_byte_offset, array.data(), sizeof(T) * N);
}

template <typename T>
void SetUniformIfPresent(std::byte* writable_data, int16_t uniform_byte_offset,
                         const T& value) {
  if (uniform_byte_offset < 0) return;
  std::memcpy(writable_data + uniform_byte_offset, &value, sizeof(T));
}

std::array<float, 4> UnpackingParamsFloat4(
    const MeshAttributeCodingParams& unpacking_transform) {
  ABSL_CHECK_EQ(unpacking_transform.components.Size(), 2);
  return {unpacking_transform.components[0].offset,
          unpacking_transform.components[0].scale,
          unpacking_transform.components[1].offset,
          unpacking_transform.components[1].scale};
}

bool IsUnpackingTransform(MeshSpecificationData::UniformId uniform_id) {
  switch (uniform_id) {
    case MeshSpecificationData::UniformId::kObjectToCanvasLinearComponent:
    case MeshSpecificationData::UniformId::kBrushColor:
    case MeshSpecificationData::UniformId::kTextureMapping:
    case MeshSpecificationData::UniformId::kTextureAnimationProgress:
    case MeshSpecificationData::UniformId::kNumTextureAnimationFrames:
    case MeshSpecificationData::UniformId::kNumTextureAnimationRows:
    case MeshSpecificationData::UniformId::kNumTextureAnimationColumns:
      return false;
    case MeshSpecificationData::UniformId::kPositionUnpackingTransform:
    case MeshSpecificationData::UniformId::kSideDerivativeUnpackingTransform:
    case MeshSpecificationData::UniformId::kForwardDerivativeUnpackingTransform:
      return true;
  }
  ABSL_LOG(FATAL) << "Got `uniform_id` with non-enumerator value: "
                  << static_cast<int>(uniform_id);
}

void InitializeAttributeUnpackingTransforms(
    const SkMeshSpecification& spec,
    absl::Span<const MeshFormat::Attribute> ink_attributes,
    absl::FunctionRef<const MeshAttributeCodingParams&(int)>
        get_attribute_unpacking_transform,
    std::byte* writable_data) {
  for (int i = 0; i < static_cast<int>(ink_attributes.size()); ++i) {
    std::optional<MeshSpecificationData::UniformId> uniform_id =
        FindUnpackingTransformUniformId(ink_attributes[i].id);
    if (!uniform_id.has_value() || !IsUnpackingTransform(*uniform_id)) {
      continue;
    }
    SetUniformIfPresent(
        writable_data, FindUniformOffset(spec, *uniform_id),
        UnpackingParamsFloat4(get_attribute_unpacking_transform(i)));
  }
}

}  // namespace

MeshUniformData::MeshUniformData(
    const SkMeshSpecification& spec,
    absl::Span<const MeshFormat::Attribute> ink_attributes,
    absl::FunctionRef<const MeshAttributeCodingParams&(int)>
        get_attribute_unpacking_transform)
    : MeshUniformData(spec) {
  InitializeAttributeUnpackingTransforms(
      spec, ink_attributes, get_attribute_unpacking_transform, WritableData());
}

void MeshUniformData::SetBrushColor(const Color& color) {
  Color::RgbaFloat rgba =
      color.InColorSpace(ColorSpace::kSrgb).AsFloat(Color::Format::kLinear);
  static_assert(sizeof(rgba) == 4 * sizeof(float));
  SetUniformIfPresent(WritableData(), brush_color_offset_, rgba);
}

void MeshUniformData::SetTextureMapping(BrushPaint::TextureMapping mapping) {
  SetUniformIfPresent(WritableData(), texture_mapping_offset_,
                      static_cast<int>(mapping));
}

void MeshUniformData::SetTextureAnimationProgress(float progress) {
  SetUniformIfPresent(WritableData(), texture_animation_progress_offset_,
                      progress);
}

void MeshUniformData::SetNumTextureAnimationFrames(int num_frames) {
  SetUniformIfPresent(WritableData(), num_texture_animation_frames_offset_,
                      num_frames);
}

void MeshUniformData::SetNumTextureAnimationRows(int num_rows) {
  SetUniformIfPresent(WritableData(), num_texture_animation_rows_offset_,
                      num_rows);
}

void MeshUniformData::SetNumTextureAnimationColumns(int num_columns) {
  SetUniformIfPresent(WritableData(), num_texture_animation_columns_offset_,
                      num_columns);
}

void MeshUniformData::SetObjectToCanvasLinearComponent(
    const AffineTransform& transform) {
  SetUniformIfPresent(WritableData(), object_to_canvas_linear_component_offset_,
                      std::array<float, 4>{transform.A(), transform.D(),
                                           transform.B(), transform.E()});
}

std::byte* MeshUniformData::WritableData() {
  if (!data_->unique() && !data_->empty()) {
    data_ = SkData::MakeWithCopy(data_->bytes(), data_->size());
  }
  return static_cast<std::byte*>(data_->writable_data());
}

}  // namespace ink::skia_native_internal
