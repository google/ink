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

#include <cstdint>

#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/strokes/internal/brush_tip_extruder.h"
#include "ink/strokes/internal/brush_tip_modeler.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/strokes/internal/stroke_outline.h"
#include "ink/strokes/internal/stroke_shape_update.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

void StrokeShapeBuilder::StartStroke(const BrushCoat& coat, float brush_size,
                                     float brush_epsilon, uint32_t noise_seed) {
  // The `tip_modeler_` and `tip_extruder_` CHECK-validate `brush_size` and
  // `brush_epsilon` being greater than zero.

  mesh_bounds_.Reset();
  outlines_.clear();

  bool is_particle_brush =
      (coat.tip.particle_gap_distance_scale != 0 ||
       coat.tip.particle_gap_duration != Duration32::Zero());
  tip_modeler_.StartStroke(&coat.tip, brush_size, noise_seed);
  tip_extruder_.StartStroke(brush_epsilon, is_particle_brush, mesh_);
}

StrokeShapeUpdate StrokeShapeBuilder::ExtendStroke(
    const StrokeInputModeler& input_modeler) {
  StrokeShapeUpdate update;
  mesh_bounds_.Reset();

  outlines_.clear();
  tip_modeler_.UpdateStroke(input_modeler.GetState(),
                            input_modeler.GetModeledInputs());
  update.Add(tip_extruder_.ExtendStroke(tip_modeler_.NewFixedTipStates(),
                                        tip_modeler_.VolatileTipStates()));
  mesh_bounds_.Add(tip_extruder_.GetBounds());
  for (const StrokeOutline& outline : tip_extruder_.GetOutlines()) {
    const absl::Span<const uint32_t>& indices = outline.GetIndices();
    if (!indices.empty()) {
      outlines_.push_back(indices);
    }
  }

  return update;
}

bool StrokeShapeBuilder::HasUnfinishedTimeBehaviors(
    const InputModelerState& input_modeler_state) const {
  return tip_modeler_.HasUnfinishedTimeBehaviors(input_modeler_state);
}

}  // namespace ink::strokes_internal
