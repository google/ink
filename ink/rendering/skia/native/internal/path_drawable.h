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

#ifndef INK_RENDERING_SKIA_NATIVE_INTERNAL_PATH_DRAWABLE_H_
#define INK_RENDERING_SKIA_NATIVE_INTERNAL_PATH_DRAWABLE_H_

#include <cstdint>

#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"
#include "ink/brush/color_function.h"
#include "ink/color/color.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/partitioned_mesh.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"

namespace ink::skia_native_internal {

// A drawable object wrapping `SkPath` and `SkPaint`.
//
// One drawable consists of one or more path objects that should all be drawn
// with the same paint.
class PathDrawable {
 public:
  // Constructs the drawable from a `MutableMesh`.
  //
  // `index_outlines` is used to retrieve path positions from the `mesh`.
  //
  // TODO: b/295166196 - Once `MutableMesh` always uses 16-bit indices, this
  // function will need to change to accept a `span<const MutableMesh>`.
  PathDrawable(const MutableMesh& mesh,
               absl::Span<const absl::Span<const uint32_t>> index_outlines,
               absl::Span<const ColorFunction> color_functions);

  // Constructs the drawable from one render group of a `PartitionedMesh`.
  PathDrawable(const PartitionedMesh& shape, uint32_t render_group_index,
               absl::Span<const ColorFunction> color_functions);

  PathDrawable() = default;
  PathDrawable(const PathDrawable&) = default;
  PathDrawable(PathDrawable&&) = default;
  PathDrawable& operator=(const PathDrawable&) = default;
  PathDrawable& operator=(PathDrawable&&) = default;
  ~PathDrawable() = default;

  // Sets the brush color to be used for this drawable. The passed-in
  // `brush_color` will be transformed by the `color_functions` passed in during
  // construction before being applied to the underlying `SkPaint`.
  void SetBrushColor(const Color& brush_color);

  void SetImageFilter(sk_sp<SkImageFilter> image_filter);

  void Draw(SkCanvas& canvas) const;

 private:
  absl::InlinedVector<SkPath, 1> paths_;
  SkPaint paint_;
  absl::InlinedVector<ColorFunction, 1> color_functions_;
};

}  // namespace ink::skia_native_internal

#endif  // INK_RENDERING_SKIA_NATIVE_INTERNAL_PATH_DRAWABLE_H_
