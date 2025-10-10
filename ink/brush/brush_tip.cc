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

#include "ink/brush/brush_tip.h"

#include <cmath>
#include <string>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "ink/brush/brush_behavior.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/vec.h"
#include "ink/types/duration.h"

namespace ink::brush_internal {

namespace {

bool BrushTipUsesColorShift(const BrushTip& tip) {
  for (const BrushBehavior& behavior : tip.behaviors) {
    for (const BrushBehavior::Node& node : behavior.nodes) {
      if (const auto* output = std::get_if<BrushBehavior::TargetNode>(&node)) {
        switch (output->target) {
          case BrushBehavior::Target::kHueOffsetInRadians:
          case BrushBehavior::Target::kSaturationMultiplier:
          case BrushBehavior::Target::kLuminosity:
            return true;
          default:
            break;
        }
      }
    }
  }
  return false;
}

}  // namespace

absl::Status ValidateBrushTipTopLevel(const BrushTip& tip) {
  if (!std::isfinite(tip.scale.x) || !std::isfinite(tip.scale.y) ||
      tip.scale.x < 0 || tip.scale.y < 0 || tip.scale == Vec{0, 0}) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Both values of `BrushTip::scale` must be finite and non-negative, and "
        "at least one value must be positive. Got %v",
        tip.scale));
  }
  if (!(tip.corner_rounding >= 0 && tip.corner_rounding <= 1)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushTip::corner_rounding` must be a value in the "
                        "interval [0, 1]. Got %f",
                        tip.corner_rounding));
  }

  if (!std::isfinite(tip.slant.ValueInRadians()) ||
      !(tip.slant >= -kQuarterTurn && tip.slant <= kQuarterTurn)) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushTip::slant` must be a finite value in the "
                        "interval [-π/2, π/2] radians ([-90, 90] degrees). "
                        "Got %v",
                        tip.slant));
  }
  if (!(tip.pinch >= 0 && tip.pinch <= 1)) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushTip::pinch` must be a value in the interval [0, 1]. Got %f",
        tip.pinch));
  }
  if (!std::isfinite(tip.rotation.ValueInRadians())) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "`BrushTip::rotation` must be finite. Got %v", tip.rotation));
  }
  if (!std::isfinite(tip.particle_gap_distance_scale) ||
      tip.particle_gap_distance_scale < 0) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushTip::particle_gap_distance_scale` must be "
                        "finite and non-negative. Got %v",
                        tip.particle_gap_distance_scale));
  }
  if (!tip.particle_gap_duration.IsFinite() ||
      tip.particle_gap_duration < Duration32::Zero()) {
    return absl::InvalidArgumentError(
        absl::StrFormat("`BrushTip::particle_gap_duration` must be finite and "
                        "non-negative. Got %v",
                        tip.particle_gap_duration));
  }
  return absl::OkStatus();
}

absl::Status ValidateBrushTip(const BrushTip& tip) {
  if (absl::Status status = ValidateBrushTipTopLevel(tip); !status.ok()) {
    return status;
  }
  for (const BrushBehavior& behavior : tip.behaviors) {
    if (absl::Status status = ValidateBrushBehavior(behavior); !status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

void AddAttributeIdsRequiredByTip(
    const BrushTip& tip,
    absl::flat_hash_set<MeshFormat::AttributeId>& attribute_ids) {
  if (BrushTipUsesColorShift(tip)) {
    attribute_ids.insert(MeshFormat::AttributeId::kColorShiftHsl);
  }
}

std::string ToFormattedString(const BrushTip& tip) {
  std::string formatted = absl::StrCat(
      "BrushTip{scale=", tip.scale, ", corner_rounding=", tip.corner_rounding);
  if (tip.slant != Angle()) {
    absl::StrAppend(&formatted, ", slant=", tip.slant);
  }
  if (tip.pinch != 0.f) {
    absl::StrAppend(&formatted, ", pinch=", tip.pinch);
  }
  if (tip.rotation != Angle()) {
    absl::StrAppend(&formatted, ", rotation=", tip.rotation);
  }
  if (tip.particle_gap_distance_scale != 0) {
    absl::StrAppend(&formatted, ", particle_gap_distance_scale=",
                    tip.particle_gap_distance_scale);
  }
  if (tip.particle_gap_duration != Duration32::Zero()) {
    absl::StrAppend(&formatted,
                    ", particle_gap_duration=", tip.particle_gap_duration);
  }
  if (!tip.behaviors.empty()) {
    absl::StrAppend(&formatted, ", behaviors={",
                    absl::StrJoin(tip.behaviors, ", "), "}");
  }
  formatted.push_back('}');
  return formatted;
}

}  // namespace ink::brush_internal
