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

#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/brush_tip_extruder.h"
#include "ink/strokes/internal/brush_tip_modeler.h"
#include "ink/strokes/internal/stroke_outline.h"
#include "ink/strokes/internal/stroke_shape_update.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {
namespace {

bool IsWindingTextureCoat(const BrushCoat& coat) {
  // We can only handle winding textures when there is a single texture layer.
  return coat.paint.texture_layers.size() == 1 &&
         coat.paint.texture_layers[0].mapping ==
             BrushPaint::TextureMapping::kWinding;
}

}  // namespace

void StrokeShapeBuilder::StartStroke(const BrushFamily::InputModel& input_model,
                                     const BrushCoat& coat, float brush_size,
                                     float brush_epsilon, uint32_t noise_seed) {
  // The `input_modeler_`, `tip_modeler_` and `tip_extruder_` CHECK-validate
  // `brush_tip` being not null, and `brush_size` and `brush_epsilon` being
  // greater than zero.
  input_modeler_.StartStroke(input_model, brush_epsilon);

  mesh_bounds_.Reset();

  tip_count_ = coat.tips.size();
  // TODO: b/285594469 - For now, a BrushFamily always has exactly one tip, but
  // we'll want to relax this restriction.
  ABSL_CHECK_EQ(tip_count_, 1u);

  // Clear the outlines from the previous stroke and reserve space for one
  // outline per tip.
  outlines_.clear();
  outlines_.reserve(tip_count_);

  // If necessary, expand the modeler/extruder vector to the number of brush
  // tips. In order to cache all the allocations within, we never shrink this
  // vector.
  if (tip_count_ > tips_.size()) {
    tips_.resize(tip_count_);
  }

  bool is_winding_texture_brush = IsWindingTextureCoat(coat);
  for (size_t i = 0; i < tip_count_; ++i) {
    bool is_winding_texture_particle_brush =
        is_winding_texture_brush &&
        (coat.tips[i].particle_gap_distance_scale != 0 ||
         coat.tips[i].particle_gap_duration != Duration32::Zero());
    tips_[i].modeler.StartStroke(&coat.tips[i], brush_size, noise_seed);
    // TODO: b/285594469 - Once it's possible for there to be more than one tip,
    // this will need to be more careful about how the various extruders append
    // onto the same mesh.
    tips_[i].extruder.StartStroke(brush_epsilon,
                                  is_winding_texture_particle_brush, mesh_);
  }
}

StrokeShapeUpdate StrokeShapeBuilder::ExtendStroke(
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs, Duration32 current_elapsed_time) {
  input_modeler_.ExtendStroke(real_inputs, predicted_inputs,
                              current_elapsed_time);
  StrokeShapeUpdate update;
  mesh_bounds_.Reset();

  // `tips_` may also have additional elements (for allocation caching reasons),
  // which should be ignored for this brush.
  ABSL_DCHECK_GE(tips_.size(), tip_count_);

  outlines_.clear();
  for (uint32_t i = 0; i < tip_count_; ++i) {
    BrushTipModeler& tip_modeler = tips_[i].modeler;
    BrushTipExtruder& tip_extruder = tips_[i].extruder;
    tip_modeler.UpdateStroke(input_modeler_.GetState(),
                             input_modeler_.GetModeledInputs());
    update.Add(tip_extruder.ExtendStroke(tip_modeler.NewFixedTipStates(),
                                         tip_modeler.VolatileTipStates()));
    mesh_bounds_.Add(tip_extruder.GetBounds());
    for (const StrokeOutline& outline : tip_extruder.GetOutlines()) {
      const absl::Span<const uint32_t>& indices = outline.GetIndices();
      if (!indices.empty()) {
        outlines_.push_back(indices);
      }
    }
  }

  return update;
}

bool StrokeShapeBuilder::HasUnfinishedTimeBehaviors() const {
  // `tips_` may have additional elements (for allocation caching reasons),
  // which should be ignored for this brush.
  ABSL_DCHECK_GE(tips_.size(), tip_count_);
  for (uint32_t i = 0; i < tip_count_; ++i) {
    if (tips_[i].modeler.HasUnfinishedTimeBehaviors(
            input_modeler_.GetState())) {
      return true;
    }
  }
  return false;
}

}  // namespace ink::strokes_internal
