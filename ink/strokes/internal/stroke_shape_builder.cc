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

#include "ink/strokes/internal/stroke_shape_builder.h"

#include <cstddef>
#include <cstdint>

#include "absl/log/check.h"
#include "absl/types/span.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_tip.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/brush_tip_extruder.h"
#include "ink/strokes/internal/brush_tip_modeler.h"
#include "ink/strokes/internal/stroke_shape_update.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

void StrokeShapeBuilder::StartStroke(const BrushFamily::InputModel& input_model,
                                     absl::Span<const BrushTip> brush_tips,
                                     float brush_size, float brush_epsilon) {
  // The `input_modeler_`, `tip_modeler_` and `tip_extruder_` CHECK-validate
  // `brush_tip` being not null, and `brush_size` and `brush_epsilon` being
  // greater than zero.
  input_modeler_.StartStroke(input_model, brush_epsilon);

  mesh_bounds_.Reset();

  const size_t num_tips = brush_tips.size();
  // TODO: b/285594469 - For now, a BrushFamily always has exactly one tip, but
  // we'll want to relax this restriction.
  CHECK_EQ(num_tips, 1u);

  // Initialize each brush tip's outline to empty.
  outline_indices_.clear();
  outline_indices_.resize(num_tips);

  // If necessary, expand the modeler/extruder vector to the number of brush
  // tips. In order to cache all the allocations within, we never shrink this
  // vector.
  if (num_tips > tips_.size()) {
    tips_.resize(num_tips);
  }

  for (size_t i = 0; i < num_tips; ++i) {
    tips_[i].modeler.StartStroke(&brush_tips[i], brush_size);
    // TODO: b/285594469 - Once it's possible for there to be more than one tip,
    // this will need to be more careful about how the various extruders append
    // onto the same mesh.
    tips_[i].extruder.StartStroke(brush_epsilon, mesh_);
  }
}

StrokeShapeUpdate StrokeShapeBuilder::ExtendStroke(
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs, Duration32 current_elapsed_time) {
  input_modeler_.ExtendStroke(real_inputs, predicted_inputs,
                              current_elapsed_time);
  StrokeShapeUpdate update;
  mesh_bounds_.Reset();

  // `outline_indices_` always has exactly one element per brush tip, but
  // `tips_` may have additional elements (for allocation caching reasons),
  // which should be ignored for this brush.
  const uint32_t num_tips = BrushTipCount();
  DCHECK_EQ(outline_indices_.size(), num_tips);
  DCHECK_GE(tips_.size(), num_tips);

  for (uint32_t i = 0; i < num_tips; ++i) {
    BrushTipModeler& tip_modeler = tips_[i].modeler;
    BrushTipExtruder& tip_extruder = tips_[i].extruder;
    tip_modeler.UpdateStroke(input_modeler_.GetState(),
                             input_modeler_.GetModeledInputs());
    update.Add(tip_extruder.ExtendStroke(tip_modeler.NewFixedTipStates(),
                                         tip_modeler.VolatileTipStates()));
    mesh_bounds_.Add(tip_extruder.GetBounds());
    outline_indices_[i] = tip_extruder.GetOutlineIndices();
  }

  return update;
}

bool StrokeShapeBuilder::HasUnfinishedTimeBehaviors() const {
  // `tips_` may have additional elements (for allocation caching reasons),
  // which should be ignored for this brush.
  const uint32_t num_tips = BrushTipCount();
  DCHECK_GE(tips_.size(), num_tips);
  for (uint32_t i = 0; i < num_tips; ++i) {
    if (tips_[i].modeler.HasUnfinishedTimeBehaviors(
            input_modeler_.GetState())) {
      return true;
    }
  }
  return false;
}

}  // namespace ink::strokes_internal
