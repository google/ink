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

#include "ink/brush/brush_coat.h"

#include <string>
#include <variant>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/geometry/mesh_format.h"

namespace ink {
namespace brush_internal {

absl::Status ValidateBrushCoat(const BrushCoat& coat) {
  if (absl::Status status = ValidateBrushTip(coat.tip); !status.ok()) {
    return status;
  }
  if (coat.paint_preferences.empty()) {
    return absl::InvalidArgumentError(
        "BrushCoat::paint_preferences must not be empty");
  }
  for (const BrushPaint& paint : coat.paint_preferences) {
    if (absl::Status status = ValidateBrushPaint(paint); !status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

absl::flat_hash_set<MeshFormat::AttributeId> GetRequiredAttributeIds(
    const BrushCoat& coat) {
  absl::flat_hash_set<MeshFormat::AttributeId> ids = GetRequiredAttributeIds();
  ids.merge(GetRequiredAttributeIds(coat.tip));
  for (const BrushPaint& paint : coat.paint_preferences) {
    ids.merge(GetRequiredAttributeIds(paint));
  }
  return ids;
}

absl::flat_hash_set<MeshFormat::AttributeId> GetRequiredAttributeIds() {
  absl::flat_hash_set<MeshFormat::AttributeId> ids = {
      // All meshes must have a kPosition attribute.
      MeshFormat::AttributeId::kPosition,
      // The side/forward attributes are always required, in order to support
      // shader-based anti-aliasing.
      MeshFormat::AttributeId::kSideDerivative,
      MeshFormat::AttributeId::kSideLabel,
      MeshFormat::AttributeId::kForwardDerivative,
      MeshFormat::AttributeId::kForwardLabel,
      // Opacity shift is always required (even when there's no `kOpacityShift`
      // brush behavior), in order to support overlap behavior for translucent
      // colors.
      MeshFormat::AttributeId::kOpacityShift,
  };
  return ids;
}

std::string ToFormattedString(const BrushCoat& coat) {
  return absl::StrFormat(
      "BrushCoat{tip=%v, paint_preferences=%v}", coat.tip,
      absl::StrCat("{", absl::StrJoin(coat.paint_preferences, ", "), "}"));
}

}  // namespace brush_internal
}  // namespace ink
