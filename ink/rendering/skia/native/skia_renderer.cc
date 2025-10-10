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

#include "ink/rendering/skia/native/skia_renderer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/inlined_vector.h"
#include "absl/functional/overload.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/color/color.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/mesh_packing_types.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/rect.h"
#include "ink/rendering/skia/native/internal/mesh_drawable.h"
#include "ink/rendering/skia/native/internal/mesh_uniform_data.h"
#include "ink/rendering/skia/native/internal/path_drawable.h"
#include "ink/rendering/skia/native/internal/shader_cache.h"
#include "ink/rendering/skia/native/texture_bitmap_store.h"
#include "ink/strokes/in_progress_stroke.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/stroke.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkM44.h"
#include "include/core/SkMesh.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkMeshGanesh.h"

namespace ink {
namespace {

using ::ink::skia_native_internal::MeshDrawable;
using ::ink::skia_native_internal::MeshUniformData;
using ::ink::skia_native_internal::PathDrawable;

void FillTemporaryIndices(const MutableMesh& mesh,
                          std::vector<uint16_t>& temporary_indices) {
  temporary_indices.clear();
  temporary_indices.reserve(3 * mesh.TriangleCount());
  for (uint32_t i = 0; i < mesh.TriangleCount(); ++i) {
    for (uint32_t index : mesh.TriangleIndices(i)) {
      temporary_indices.push_back(index);
    }
  }
}

SkRect ToSkiaRect(const Rect& rect) {
  return SkRect::MakeLTRB(rect.XMin(), rect.YMin(), rect.XMax(), rect.YMax());
}

// Returns whether the given `paint` is compatible with the given `mesh_format`.
// This doesn't take a `BrushCoat` because at render time, we need to consider
// attributes required by just a specific `BrushPaint` preference, not all of
// the `BrushPaint` preferences in the `BrushCoat`.
bool MeshFormatSupports(const MeshFormat& mesh_format,
                        const BrushPaint& paint) {
  // Gather all the attributes that are required by the paint.
  absl::flat_hash_set<MeshFormat::AttributeId> required_attribute_ids;
  brush_internal::AddAttributeIdsRequiredByPaint(paint, required_attribute_ids);
  // TODO: b/346530293 - Check for attributes that are always required (e.g.
  //   position) and those that may be required by the tip (e.g. HSL shift).
  //   That can happen either at a higher level (e.g. not inside a loop over the
  //   paint preferences), or earlier such as when a `Stroke` is constructed
  //   with a preexisting `PartitionedMesh`.

  // Check if all required attributes are present in the mesh format.
  for (const auto& attr : mesh_format.Attributes()) {
    required_attribute_ids.erase(attr.id);
  }
  return required_attribute_ids.empty();
}

// Returns whether mesh rendering is supported for the given `paint`, `tip` and
// `mesh_format`.
bool IsMeshRenderingSupported(GrDirectContext* const absl_nullable context,
                              const BrushPaint& paint,
                              const MeshFormat& mesh_format) {
  return context != nullptr &&
         brush_internal::AllowsSelfOverlapMode(
             paint, BrushPaint::SelfOverlap::kAccumulate) &&
         MeshFormatSupports(mesh_format, paint);
}

// Returns whether path rendering is supported for the given `paint`.
bool IsPathRenderingSupported(const BrushPaint& paint) {
  return std::all_of(paint.texture_layers.begin(), paint.texture_layers.end(),
                     [](const BrushPaint::TextureLayer& layer) {
                       return layer.mapping ==
                              BrushPaint::TextureMapping::kTiling;
                     }) &&
         brush_internal::AllowsSelfOverlapMode(
             paint, BrushPaint::SelfOverlap::kDiscard);
}

absl::StatusOr<MeshDrawable> CreateAndInitializeMeshDrawable(
    const StrokeInputBatch& inputs, const Brush& brush,
    const BrushPaint& brush_paint,
    skia_native_internal::ShaderCache& shader_cache,
    MeshUniformData&& uniform_data, sk_sp<SkMeshSpecification> specification,
    absl::InlinedVector<MeshDrawable::Partition, 1>&& partitions) {
  absl::StatusOr<sk_sp<SkShader>> shader =
      shader_cache.GetShaderForPaint(brush_paint, brush.GetSize(), inputs);
  if (!shader.ok()) return shader.status();
  absl::StatusOr<MeshDrawable> mesh_drawable = MeshDrawable::Create(
      specification, shader_cache.GetBlenderForPaint(brush_paint),
      *std::move(shader), brush_paint.color_functions, partitions,
      std::move(uniform_data));
  if (!mesh_drawable.ok()) return mesh_drawable.status();
  mesh_drawable->SetBrushColor(brush.GetColor());
  // Currently, validation enforces that these values are all the same for all
  // texture layers in a given paint. If there are no texture layers, use the
  // default "tiling" behavior. When there is no animation, the default of 1 for
  // frames, rows, and columns are reasonable placeholder values, those will be
  // set if present in the mesh format.
  BrushPaint::TextureLayer texture_layer = brush_paint.texture_layers.empty()
                                               ? BrushPaint::TextureLayer{}
                                               : brush_paint.texture_layers[0];
  // TODO: b/375203215 - Get rid of this uniform once we are able to mix
  // different texture mapping modes in a single `BrushPaint`.
  mesh_drawable->SetTextureMapping(texture_layer.mapping);
  mesh_drawable->SetNumTextureAnimationFrames(texture_layer.animation_frames);
  mesh_drawable->SetNumTextureAnimationRows(texture_layer.animation_rows);
  mesh_drawable->SetNumTextureAnimationColumns(texture_layer.animation_columns);
  // Actual updates of this need to be done on draw.
  mesh_drawable->SetTextureAnimationProgress(0.0f);
  return *std::move(mesh_drawable);
}

}  // namespace

SkiaRenderer::SkiaRenderer(
    absl_nullable std::shared_ptr<TextureBitmapStore> texture_provider)
    : texture_provider_(std::move(texture_provider)),
      shader_cache_(texture_provider_.get()) {}

absl::StatusOr<SkiaRenderer::Drawable> SkiaRenderer::CreateDrawable(
    GrDirectContext* absl_nullable context, const InProgressStroke& stroke,
    const AffineTransform& object_to_canvas) {
  const Brush* brush = stroke.GetBrush();
  if (brush == nullptr) {
    return Drawable(object_to_canvas, {});
  }

  uint32_t num_coats = brush->CoatCount();
  absl::InlinedVector<Drawable::Implementation, 1> drawables;
  drawables.reserve(num_coats);
  for (uint32_t coat_index = 0; coat_index < num_coats; ++coat_index) {
    if (stroke.GetMeshBounds(coat_index).IsEmpty()) continue;
    absl::StatusOr<Drawable::Implementation> drawable =
        CreateDrawableImpl(context, stroke, coat_index, *brush);
    if (!drawable.ok()) return drawable.status();
    drawables.push_back(*std::move(drawable));
  }
  return Drawable(object_to_canvas, std::move(drawables));
}

absl::StatusOr<SkiaRenderer::Drawable::Implementation>
SkiaRenderer::CreateDrawableImpl(GrDirectContext* const absl_nullable context,
                                 const InProgressStroke& stroke,
                                 uint32_t coat_index, const Brush& brush) {
  const BrushCoat& coat = brush.GetCoats()[coat_index];
  for (const BrushPaint& paint : coat.paint_preferences) {
    if (IsMeshRenderingSupported(context, paint,
                                 stroke.GetMeshFormat(coat_index))) {
      ABSL_CHECK(context != nullptr);
      // Mesh rendering requires a `GrDirectContext`, can handle any texture
      // mapping mode, and supports accumulating self overlap.
      return CreateMeshDrawable(context, stroke, coat_index, paint, brush);
    } else if (IsPathRenderingSupported(paint)) {
      // Path rendering can only handle tiled textures and discarding
      // self overlap, but doesn't require a `GrDirectContext`.
      return CreatePathDrawable(stroke, coat_index, paint, brush);
    }
  }
  return absl::InvalidArgumentError(
      "No supported paint preferences for this brush coat.");
}

absl::StatusOr<MeshDrawable> SkiaRenderer::CreateMeshDrawable(
    GrDirectContext* const absl_nonnull context, const InProgressStroke& stroke,
    uint32_t coat_index, const BrushPaint& brush_paint, const Brush& brush) {
  absl::StatusOr<sk_sp<SkMeshSpecification>> specification =
      specification_cache_.GetFor(stroke);
  if (!specification.ok()) return specification.status();

  const MutableMesh& mesh = stroke.GetMesh(coat_index);
  if (mesh.VertexCount() >= std::numeric_limits<uint16_t>::max()) {
    return absl::UnimplementedError(
        "Strokes requiring at least 2^16 indices are not supported yet.");
  }

  absl::Span<const std::byte> vertex_data = mesh.RawVertexData();
  FillTemporaryIndices(mesh, temporary_indices_);

  MeshUniformData uniform_data(**specification);
  return CreateAndInitializeMeshDrawable(
      stroke.GetInputs(), brush, brush_paint, shader_cache_,
      std::move(uniform_data), *std::move(specification),
      {{
          .vertex_buffer = SkMeshes::MakeVertexBuffer(
              context, vertex_data.data(), vertex_data.size()),
          .index_buffer = SkMeshes::MakeIndexBuffer(
              context, temporary_indices_.data(),
              temporary_indices_.size() * sizeof(uint16_t)),
          .vertex_count = static_cast<int32_t>(mesh.VertexCount()),
          .index_count = static_cast<int32_t>(3 * mesh.TriangleCount()),
          .bounds = ToSkiaRect(*stroke.GetMeshBounds(coat_index).AsRect()),
      }});
}

absl::StatusOr<PathDrawable> SkiaRenderer::CreatePathDrawable(
    const InProgressStroke& stroke, uint32_t coat_index,
    const BrushPaint& brush_paint, const Brush& brush) {
  PathDrawable drawable(stroke.GetMesh(coat_index),
                        stroke.GetCoatOutlines(coat_index),
                        brush_paint.color_functions);
  drawable.SetBrushColor(brush.GetColor());
  return drawable;
}

absl::StatusOr<SkiaRenderer::Drawable> SkiaRenderer::CreateDrawable(
    GrDirectContext* const absl_nullable context, const Stroke& stroke,
    const AffineTransform& object_to_canvas) {
  const PartitionedMesh& stroke_shape = stroke.GetShape();
  if (stroke_shape.RenderGroupCount() == 0) {
    return Drawable(object_to_canvas, {});
  }

  const Brush& brush = stroke.GetBrush();
  uint32_t num_coats = brush.CoatCount();
  // This is guaranteed by the `Stroke` class.
  ABSL_DCHECK_EQ(stroke_shape.RenderGroupCount(), num_coats);

  absl::InlinedVector<Drawable::Implementation, 1> drawables;
  drawables.reserve(num_coats);
  for (uint32_t coat_index = 0; coat_index < num_coats; ++coat_index) {
    absl::Span<const Mesh> meshes = stroke_shape.RenderGroupMeshes(coat_index);
    if (meshes.empty()) continue;

    absl::StatusOr<Drawable::Implementation> drawable =
        CreateDrawableImpl(context, stroke, coat_index, brush);
    if (!drawable.ok()) return drawable.status();
    drawables.push_back(*std::move(drawable));
  }

  return Drawable(object_to_canvas, std::move(drawables));
}

absl::StatusOr<SkiaRenderer::Drawable::Implementation>
SkiaRenderer::CreateDrawableImpl(GrDirectContext* const absl_nullable context,
                                 const Stroke& stroke, uint32_t coat_index,
                                 const Brush& brush) {
  const BrushCoat& coat = brush.GetCoats()[coat_index];
  for (const BrushPaint& paint : coat.paint_preferences) {
    if (IsMeshRenderingSupported(
            context, paint, stroke.GetShape().RenderGroupFormat(coat_index))) {
      ABSL_CHECK(context != nullptr);
      // Mesh rendering requires a `GrDirectContext` and particular mesh
      // attributes, but it can handle any texture mapping mode and it supports
      // accumulating self overlap.
      return CreateMeshDrawable(context, stroke, coat_index, paint);
    } else if (IsPathRenderingSupported(paint)) {
      // Path rendering can only handle tiled textures and discarding
      // self overlap, but doesn't require a `GrDirectContext` or any
      // paint-specific mesh attributes.
      return CreatePathDrawable(stroke, coat_index, paint, brush);
    }
  }
  return absl::InvalidArgumentError(
      "No supported paint preferences for this brush coat.");
}

absl::StatusOr<PathDrawable> SkiaRenderer::CreatePathDrawable(
    const Stroke& stroke, uint32_t coat_index, const BrushPaint& brush_paint,
    const Brush& brush) {
  PathDrawable drawable(stroke.GetShape(), coat_index,
                        brush_paint.color_functions);
  drawable.SetBrushColor(brush.GetColor());
  return drawable;
}

absl::StatusOr<MeshDrawable> SkiaRenderer::CreateMeshDrawable(
    GrDirectContext* const absl_nonnull context, const Stroke& stroke,
    uint32_t coat_index, const BrushPaint& brush_paint) {
  const PartitionedMesh& stroke_shape = stroke.GetShape();
  absl::Span<const Mesh> meshes = stroke_shape.RenderGroupMeshes(coat_index);

  // TODO: b/284117747 - Pass `brush.GetCoats()[coat_index].paint` to the
  // `specification_cache_`.
  absl::StatusOr<sk_sp<SkMeshSpecification>> specification =
      specification_cache_.GetForStroke(stroke_shape, coat_index);
  if (!specification.ok()) return specification.status();

  absl::InlinedVector<MeshDrawable::Partition, 1> partitions;
  partitions.reserve(meshes.size());
  for (const Mesh& mesh : meshes) {
    absl::Span<const std::byte> vertex_data = mesh.RawVertexData();
    absl::Span<const std::byte> index_data = mesh.RawIndexData();
    partitions.push_back({
        .vertex_buffer = SkMeshes::MakeVertexBuffer(context, vertex_data.data(),
                                                    vertex_data.size()),
        .index_buffer = SkMeshes::MakeIndexBuffer(context, index_data.data(),
                                                  index_data.size()),
        .vertex_count = static_cast<int32_t>(mesh.VertexCount()),
        .index_count = static_cast<int32_t>(3 * mesh.TriangleCount()),
        .bounds = ToSkiaRect(*mesh.Bounds().AsRect()),
    });
  }

  const Mesh& first_mesh = meshes.front();
  auto get_attribute_unpacking_transform =
      [&first_mesh](int attribute_index) -> const MeshAttributeCodingParams& {
    return first_mesh.VertexAttributeUnpackingParams(attribute_index);
  };

  MeshUniformData uniform_data(**specification,
                               first_mesh.Format().Attributes(),
                               get_attribute_unpacking_transform);

  return CreateAndInitializeMeshDrawable(
      stroke.GetInputs(), stroke.GetBrush(), brush_paint, shader_cache_,
      std::move(uniform_data), *std::move(specification),
      std::move(partitions));
}

absl::Status SkiaRenderer::Draw(GrDirectContext* absl_nullable context,
                                const InProgressStroke& stroke,
                                const AffineTransform& object_to_canvas,
                                SkCanvas& canvas) {
  // TODO: b/286547863 - Implement `RenderCache` to save and update the created
  // drawable inside the stroke.

  auto drawable = CreateDrawable(context, stroke, object_to_canvas);
  if (!drawable.ok()) return drawable.status();
  drawable->Draw(canvas);
  return absl::OkStatus();
}

absl::Status SkiaRenderer::Draw(GrDirectContext* absl_nullable context,
                                const Stroke& stroke,
                                const AffineTransform& object_to_canvas,
                                SkCanvas& canvas) {
  // TODO: b/286547863 - Implement `RenderCache` to save and update the created
  // drawable inside the stroke.

  auto drawable = CreateDrawable(context, stroke, object_to_canvas);
  if (!drawable.ok()) return drawable.status();
  drawable->Draw(canvas);
  return absl::OkStatus();
}

namespace {

SkM44 ToSkiaM44(const AffineTransform& t) {
  // The constructor parameters are documented to be in row-major order.
  return SkM44(t.A(), t.B(), 0, t.C(),  //
               t.D(), t.E(), 0, t.F(),  //
               0, 0, 1, 0,              //
               0, 0, 0, 1);             //
}

}  // namespace

void SkiaRenderer::Drawable::Draw(SkCanvas& canvas) const {
  canvas.setMatrix(ToSkiaM44(object_to_canvas_));
  for (const Implementation& impl : drawable_implementations_) {
    std::visit([&canvas](const auto& drawable) { drawable.Draw(canvas); },
               impl);
  }
}

SkiaRenderer::Drawable::Drawable(
    const AffineTransform& object_to_canvas,
    absl::InlinedVector<Implementation, 1> drawable_impls)
    : drawable_implementations_(std::move(drawable_impls)) {
  SetObjectToCanvas(object_to_canvas);
}

void SkiaRenderer::Drawable::SetObjectToCanvas(
    const AffineTransform& object_to_canvas) {
  object_to_canvas_ = object_to_canvas;
  for (Implementation& drawable_impl : drawable_implementations_) {
    std::visit(absl::Overload(
                   [&object_to_canvas](MeshDrawable& drawable) {
                     drawable.SetObjectToCanvas(object_to_canvas);
                   },
                   [](PathDrawable& drawable) {}),
               drawable_impl);
  }
}

void SkiaRenderer::Drawable::SetBrushColor(const Color& brush_color) {
  for (Implementation& drawable_impl : drawable_implementations_) {
    std::visit(
        [&brush_color](auto& drawable) { drawable.SetBrushColor(brush_color); },
        drawable_impl);
  }
}

void SkiaRenderer::Drawable::SetImageFilter(sk_sp<SkImageFilter> image_filter) {
  for (Implementation& drawable_impl : drawable_implementations_) {
    std::visit([&image_filter](
                   auto& drawable) { drawable.SetImageFilter(image_filter); },
               drawable_impl);
  }
}

}  // namespace ink
