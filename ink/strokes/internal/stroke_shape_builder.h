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

#ifndef INK_STROKES_INTERNAL_STROKE_SHAPE_BUILDER_H_
#define INK_STROKES_INTERNAL_STROKE_SHAPE_BUILDER_H_

#include <cstdint>

#include "absl/container/inlined_vector.h"
#include "absl/types/span.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_tip.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/brush_tip_extruder.h"
#include "ink/strokes/internal/brush_tip_modeler.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/strokes/internal/stroke_shape_update.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/types/duration.h"

namespace ink::strokes_internal {

// A `StrokeShapeBuilder` handles the end-to-end operation of turning
// `StrokeInput` and `Brush` into a `MutableMesh` with associated outlines.
//
// It is a distinct type from the public `InProgressStroke` because unlike that
// type, the `StrokeShapeBuilder`:
//   1. Does NOT save a copy of the `Brush`.
//   2. Does NOT save incoming incremental input to build a `StrokeInputBatch`
//      for the complete stroke.
//   3. Does NOT perform `Status`-returning validation that the incoming
//      `StrokeInputBatch` objects together form a valid input sequence.
//
// This is because the `StrokeShapeBuilder` is made for both the incremental
// mesh creation done by `InProgressStroke` and the all-at-once mesh creation
// done by `Stroke`. The `Stroke` object will already have both the `Brush` and
// complete `StrokeInputBatch`.
class StrokeShapeBuilder {
 public:
  StrokeShapeBuilder();
  StrokeShapeBuilder(const StrokeShapeBuilder&) = delete;
  StrokeShapeBuilder& operator=(const StrokeShapeBuilder&) = delete;
  StrokeShapeBuilder(StrokeShapeBuilder&&) = default;
  StrokeShapeBuilder& operator=(StrokeShapeBuilder&&) = default;
  ~StrokeShapeBuilder() = default;

  // Clears any ongoing stroke geometry and starts a new stroke with the given
  // brush tips, size, and epsilon.
  //
  // Each `BrushTip` in `brush_tips` must remain valid and unchanged for the
  // duration of the stroke. `brush_size` and `brush_epsilon` must be greater
  // than zero. See also `Brush::Create()` for detailed documentation. This
  // function must be called before calling `ExtendStroke()`.
  //
  // For now, `brush_tips` must contain exactly one brush tip.
  // TODO: b/285594469 - Lift this restriction.
  void StartStroke(const BrushFamily::InputModel& input_model,
                   absl::Span<const BrushTip> brush_tips, float brush_size,
                   float brush_epsilon);

  // Adds new incremental real and predicted inputs to the current stroke.
  //
  // The `real_inputs` and `predicted_inputs` parameters must have already been
  // checked to form a valid input sequence together with any previously
  // passed-in inputs for the current stroke.
  //
  // The `current_elapsed_time` should be the duration from the start of the
  // stroke until "now". The `elapsed_time` of inputs may be "in the future"
  // relative to this duration.
  //
  // CHECK-fails if `FinishStrokeInputs()` has been called for this stroke and
  // either `real_inputs` or `predicted_inputs` is non-empty.
  StrokeShapeUpdate ExtendStroke(const StrokeInputBatch& real_inputs,
                                 const StrokeInputBatch& predicted_inputs,
                                 Duration32 current_elapsed_time);

  // Returns true if any of the brush tips for this builder have any behaviors
  // whose source values could continue to change with the further passage of
  // time (even in the absence of any new inputs).
  bool HasUnfinishedTimeBehaviors() const;

  // Indicates that the inputs for the current stroke are finished. This method
  // is idempotent.
  void FinishStrokeInputs();

  const MutableMesh& GetMesh() const;

  const Envelope& GetMeshBounds() const;

  // Returns spans of outline indices, one for each of the outlines generated
  // for the stroke.
  //
  // The return value will be empty if no stroke has been started. See the
  // public `InProgressStroke::GetIndexOutlines()` for more details.
  absl::Span<const absl::Span<const uint32_t>> GetIndexOutlines() const;

 private:
  // Returns the number of brush tips being used to extrude the current shape.
  uint32_t BrushTipCount() const;

  StrokeInputModeler input_modeler_;
  MutableMesh mesh_;
  Envelope mesh_bounds_;

  // The outline for each brush tip. The size of this vector is always equal to
  // the number of brush tips.
  absl::InlinedVector<absl::Span<const uint32_t>, 1> outline_indices_;

  struct BrushTipModelerAndExtruder {
    BrushTipModeler modeler;
    BrushTipExtruder extruder;
  };
  // The modeler/extruder for each brush tip. In order to cache allocations, we
  // never shrink this vector; its size is always at least the number of brush
  // tips, but may be more if a previous brush had more tips.
  absl::InlinedVector<BrushTipModelerAndExtruder, 1> tips_;
};

// ---------------------------------------------------------------------------
//                     Implementation details below

inline StrokeShapeBuilder::StrokeShapeBuilder()
    : mesh_(StrokeVertex::FullMeshFormat()) {}

inline void StrokeShapeBuilder::FinishStrokeInputs() {
  input_modeler_.FinishStrokeInputs();
}

inline const MutableMesh& StrokeShapeBuilder::GetMesh() const { return mesh_; }

inline const Envelope& StrokeShapeBuilder::GetMeshBounds() const {
  return mesh_bounds_;
}

inline absl::Span<const absl::Span<const uint32_t>>
StrokeShapeBuilder::GetIndexOutlines() const {
  return outline_indices_;
}

inline uint32_t StrokeShapeBuilder::BrushTipCount() const {
  return outline_indices_.size();
}

}  // namespace ink::strokes_internal

#endif  // INK_STROKES_INTERNAL_STROKE_SHAPE_BUILDER_H_
