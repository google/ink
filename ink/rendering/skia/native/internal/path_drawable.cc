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

#include "ink/rendering/skia/native/internal/path_drawable.h"

#include <cstdint>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/brush/color_function.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_index_types.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPathTypes.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkShader.h"

namespace ink::skia_native_internal {
namespace {

// Appends a closed polygon contour to `path_builder` using `outline_indices` to
// retrieve vertex positions from `mesh`.
void AppendOutlineToPath(const MutableMesh& mesh,
                         absl::Span<const uint32_t> outline_indices,
                         SkPathBuilder& path_builder) {
  ABSL_DCHECK(!outline_indices.empty());

  Point position = mesh.VertexPosition(outline_indices.front());
  outline_indices.remove_prefix(1);

  path_builder.moveTo(position.x, position.y);
  for (uint32_t index : outline_indices) {
    position = mesh.VertexPosition(index);
    path_builder.lineTo(position.x, position.y);
  }
  path_builder.close();
}

// Appends a closed polygon contour to `path_builder` using
// `group_outline_indices` to retrieve vertex positions from the meshes in
// `mesh_group`.
void AppendOutlineToPath(
    absl::Span<const Mesh> mesh_group,
    absl::Span<const VertexIndexPair> group_outline_indices,
    SkPathBuilder& path_builder) {
  ABSL_DCHECK(!group_outline_indices.empty());

  Point position =
      mesh_group[group_outline_indices.front().mesh_index].VertexPosition(
          group_outline_indices.front().vertex_index);
  group_outline_indices.remove_prefix(1);

  path_builder.moveTo(position.x, position.y);
  for (VertexIndexPair index_pair : group_outline_indices) {
    position = mesh_group[index_pair.mesh_index].VertexPosition(
        index_pair.vertex_index);
    path_builder.lineTo(position.x, position.y);
  }
  path_builder.close();
}

void SetPaintDefaultsForPath(SkPaint& paint) {
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
}

}  // namespace

PathDrawable::PathDrawable(
    const MutableMesh& mesh,
    absl::Span<const absl::Span<const uint32_t>> index_outlines,
    absl::Span<const ColorFunction> color_functions, sk_sp<SkShader> shader,
    SkBlendMode color_filter_blend_mode)
    : color_functions_(color_functions.begin(), color_functions.end()),
      color_filter_blend_mode_(color_filter_blend_mode) {
  SkPathBuilder path_builder;
  path_builder.setFillType(SkPathFillType::kWinding);
  for (absl::Span<const uint32_t> indices : index_outlines) {
    if (indices.empty()) continue;

    AppendOutlineToPath(mesh, indices, path_builder);
  }
  path_ = path_builder.detach();
  SetPaintDefaultsForPath(paint_);
  paint_.setShader(std::move(shader));
}

PathDrawable::PathDrawable(const PartitionedMesh& shape,
                           uint32_t render_group_index,
                           absl::Span<const ColorFunction> color_functions,
                           sk_sp<SkShader> shader,
                           SkBlendMode color_filter_blend_mode)
    : color_functions_(color_functions.begin(), color_functions.end()),
      color_filter_blend_mode_(color_filter_blend_mode) {
  SkPathBuilder path_builder;
  path_builder.setFillType(SkPathFillType::kWinding);
  absl::Span<const Mesh> mesh_group =
      shape.RenderGroupMeshes(render_group_index);
  for (uint32_t i = 0; i < shape.OutlineCount(render_group_index); ++i) {
    absl::Span<const VertexIndexPair> indices =
        shape.Outline(render_group_index, i);
    if (indices.empty()) continue;

    AppendOutlineToPath(mesh_group, indices, path_builder);
  }
  path_ = path_builder.detach();
  SetPaintDefaultsForPath(paint_);
  paint_.setShader(std::move(shader));
}

void PathDrawable::SetBrushColor(const Color& brush_color) {
  Color::RgbaFloat c = ColorFunction::ApplyAll(color_functions_, brush_color)
                           .InColorSpace(ColorSpace::kSrgb)
                           .AsFloat(Color::Format::kLinear);
  sk_sp<SkColorSpace> srgb_linear = SkColorSpace::MakeSRGBLinear();
  SkColor4f sk_color = {.fR = c.r, .fG = c.g, .fB = c.b, .fA = c.a};
  if (paint_.getShader() != nullptr) {
    paint_.setColor(SK_ColorWHITE);
    paint_.setColorFilter(SkColorFilters::Blend(
        sk_color, std::move(srgb_linear), color_filter_blend_mode_));
  } else {
    paint_.setColor(sk_color, srgb_linear.get());
  }
}

void PathDrawable::SetImageFilter(sk_sp<SkImageFilter> image_filter) {
  paint_.setImageFilter(image_filter);
}

void PathDrawable::Draw(SkCanvas& canvas) const {
  canvas.drawPath(path_, paint_);
}

}  // namespace ink::skia_native_internal
