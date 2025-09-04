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

#ifndef INK_STROKES_BRUSH_BRUSH_COAT_H_
#define INK_STROKES_BRUSH_BRUSH_COAT_H_

#include <string>

#include "absl/container/flat_hash_set.h"
#include "absl/container/inlined_vector.h"
#include "absl/status/status.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/geometry/mesh_format.h"

namespace ink {

// A `BrushCoat` represents one coat of ink applied by a brush. It includes a
// `BrushTip` that describes the structure of that coat, and a non-empty list of
// possible `BrushPaint` objects - each one describes how to render the coat
// structure, and the one `BrushPaint` that is actually used is the first one in
// the list that is compatible with the device and renderer. Multiple
// `BrushCoat`s can be combined within a single brush; when a stroke drawn by a
// multi-coat brush is rendered, each coat of ink will be drawn entirely atop
// the previous coat, even if the stroke crosses over itself, as though each
// coat were painted in its entirety one at a time.
struct BrushCoat {
  BrushTip tip;
  absl::InlinedVector<BrushPaint, 1> paint_preferences = {BrushPaint{}};
};

namespace brush_internal {

// Determines whether the given BrushCoat struct is valid to be used in a
// BrushFamily, and returns an error if not.
absl::Status ValidateBrushCoat(const BrushCoat& coat);

// Adds the mesh attribute IDs that are required to properly render a mesh
// made with this brush coat to the given `attribute_ids` set. This will always
// include `kPosition` and certain other attribute IDs (`kSideDerivative`,
// `kSideLabel`, `kForwardDerivative`, `kForwardLabel`, and `kOpacityShift`),
// and may also include additional attribute IDs depending on the tip and paint
// settings. Note that this includes the attributes required by any of the paint
// preferences, not just the one that would actually be used for rendering.
void AddAttributeIdsRequiredByCoat(
    const BrushCoat& coat,
    absl::flat_hash_set<MeshFormat::AttributeId>& attribute_ids);

// Adds the mesh attribute IDs that are required to properly render a mesh
// made with any brush coat. This will always include `kPosition` and certain
// other attribute IDs. Note that this does not include the attributes that may
// be required by a specific tip or paint, which would need to be queried
// separately.
void AddRequiredAttributeIds(
    absl::flat_hash_set<MeshFormat::AttributeId>& attribute_ids);

std::string ToFormattedString(const BrushCoat& coat);

}  // namespace brush_internal

template <typename Sink>
void AbslStringify(Sink& sink, const BrushCoat& coat) {
  sink.Append(brush_internal::ToFormattedString(coat));
}

}  // namespace ink

#endif  // INK_STROKES_BRUSH_BRUSH_COAT_H_
