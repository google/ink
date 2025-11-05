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

#include "ink/strokes/in_progress_stroke.h"

#include <cstdint>
#include <optional>

#include "absl/container/flat_hash_set.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_coat.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/strokes/input/internal/stroke_input_validation_helpers.h"
#include "ink/strokes/input/stroke_input.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/internal/modeled_stroke_input.h"
#include "ink/strokes/internal/stroke_input_modeler.h"
#include "ink/strokes/internal/stroke_shape_update.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "ink/strokes/stroke.h"
#include "ink/types/duration.h"

namespace ink {

using ::ink::stroke_input_internal::ValidateAdvancingXYT;
using ::ink::stroke_input_internal::ValidateConsistentAttributes;
using ::ink::strokes_internal::StrokeShapeUpdate;
using ::ink::strokes_internal::StrokeVertex;

void InProgressStroke::Clear() {
  brush_.reset();
  queued_real_inputs_.Clear();
  queued_predicted_inputs_.Clear();
  processed_inputs_.Clear();
  real_input_count_ = 0;
  current_elapsed_time_ = Duration32::Zero();
  updated_region_.Reset();
  inputs_are_finished_ = true;
}

void InProgressStroke::Start(const Brush& brush, uint32_t noise_seed) {
  Clear();
  brush_ = brush;
  processed_inputs_.SetNoiseSeed(noise_seed);
  inputs_are_finished_ = false;

  absl::Span<const BrushCoat> coats = brush_->GetCoats();
  uint32_t num_coats = coats.size();
  // If necessary, expand the builders vector to the number of brush coats. In
  // order to cache all the allocations within, we never shrink this vector.
  if (shape_builders_.size() < num_coats) {
    shape_builders_.resize(num_coats);
  }

  input_modeler_.StartStroke(brush_->GetFamily().GetInputModel(),
                             brush_->GetEpsilon());
  for (uint32_t i = 0; i < num_coats; ++i) {
    shape_builders_[i].StartStroke(coats[i], brush_->GetSize(),
                                   brush_->GetEpsilon(), noise_seed);
  }
}

absl::Status InProgressStroke::EnqueueInputs(
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs) {
  if (!brush_.has_value()) {
    return absl::FailedPreconditionError(
        "`Start()` must be called at least once prior to calling "
        "`EnqueueInputs()`.");
  }
  if (InputsAreFinished()) {
    return absl::FailedPreconditionError(
        "Cannot call `EnqueueInputs()` after `FinishInputs()` until `Start()` "
        "is called again.");
  }

  // Separately validate the new inputs first, so that the calls to
  // `StrokeInputBatch::Append` below always succeed. This helps ensure that we
  // don't modify the `InProgressStroke` if an error occurs. Since we expect
  // the subsequent calls to `Append` to succeed, we log an error if they don't.
  if (auto status = ValidateNewInputsAttributes(real_inputs, predicted_inputs);
      !status.ok()) {
    return status;
  }

  ABSL_CHECK_OK(queued_real_inputs_.Append(
      real_inputs, GetFirstValidInput(real_inputs), real_inputs.Size()));

  queued_predicted_inputs_.Clear();
  ABSL_CHECK_OK(queued_predicted_inputs_.Append(
      predicted_inputs, GetFirstValidInput(predicted_inputs),
      predicted_inputs.Size()));

  return absl::OkStatus();
}

absl::Status InProgressStroke::UpdateShape(Duration32 current_elapsed_time) {
  if (!brush_.has_value()) {
    return absl::FailedPreconditionError(
        "`Start()` must be called at least once prior to calling "
        "`UpdateShape()`.");
  }

  if (auto status = ValidateNewElapsedTime(current_elapsed_time);
      !status.ok()) {
    return status;
  }
  if (inputs_are_finished_ || !queued_real_inputs_.IsEmpty() ||
      !queued_predicted_inputs_.IsEmpty()) {
    // Erase any old predicted inputs.
    processed_inputs_.Erase(real_input_count_);
  }

  if (absl::Status status = processed_inputs_.Append(queued_real_inputs_);
      !status.ok()) {
    ABSL_LOG(ERROR)
        << "Failed to appened queued real inputs to processed inputs "
           "after validation: "
        << status;
    return status;
  }
  real_input_count_ += queued_real_inputs_.Size();

  if (absl::Status status = processed_inputs_.Append(queued_predicted_inputs_);
      !status.ok()) {
    ABSL_LOG(ERROR)
        << "Failed to appened queued predicted inputs to processed inputs "
           "after validation: "
        << status;
    return status;
  }

  current_elapsed_time_ = current_elapsed_time;

  input_modeler_.ExtendStroke(queued_real_inputs_, queued_predicted_inputs_,
                              current_elapsed_time);
  uint32_t num_coats = BrushCoatCount();
  for (uint32_t i = 0; i < num_coats; ++i) {
    StrokeShapeUpdate update = shape_builders_[i].ExtendStroke(input_modeler_);

    updated_region_.Add(update.region);
    // TODO: b/286547863 - Pass `update.first_vertex_offset` and
    // `update.first_index_offset` to a `RenderCache` member once implemented.
  }

  queued_real_inputs_.Clear();
  queued_predicted_inputs_.Clear();
  return absl::OkStatus();
}

bool InProgressStroke::NeedsUpdate() const {
  if (!queued_real_inputs_.IsEmpty() || !queued_predicted_inputs_.IsEmpty()) {
    return true;
  }
  return ChangesWithTime();
}

bool InProgressStroke::ChangesWithTime() const {
  const strokes_internal::InputModelerState& input_modeler_state =
      input_modeler_.GetState();
  uint32_t num_coats = BrushCoatCount();
  for (uint32_t coat_index = 0; coat_index < num_coats; ++coat_index) {
    if (shape_builders_[coat_index].HasUnfinishedTimeBehaviors(
            input_modeler_state)) {
      return true;
    }
  }
  return false;
}

int InProgressStroke::GetFirstValidInput(
    const StrokeInputBatch& new_inputs) const {
  if (queued_real_inputs_.IsEmpty() && real_input_count_ == 0) return 0;

  const StrokeInput& last_old_real_input =
      queued_real_inputs_.IsEmpty()
          ? processed_inputs_.Get(real_input_count_ - 1)
          : queued_real_inputs_.Last();

  for (int i = 0; i < new_inputs.Size(); ++i) {
    if (ValidateAdvancingXYT(last_old_real_input, new_inputs.Get(i)).ok()) {
      return i;
    }
  }

  return new_inputs.Size();
}

absl::Status InProgressStroke::ValidateNewInputsAttributes(
    const StrokeInputBatch& real_inputs,
    const StrokeInputBatch& predicted_inputs) const {
  // If there are no new inputs, there's nothing to validate.
  if (real_inputs.IsEmpty() && predicted_inputs.IsEmpty()) {
    return absl::OkStatus();
  }

  // If there's a previous real input, check that against the first new input.
  if (!queued_real_inputs_.IsEmpty() || real_input_count_ != 0) {
    const StrokeInput& last_old_real_input =
        queued_real_inputs_.IsEmpty()
            ? processed_inputs_.Get(real_input_count_ - 1)
            : queued_real_inputs_.Last();
    const StrokeInput& first_new_input =
        real_inputs.IsEmpty() ? predicted_inputs.First() : real_inputs.First();
    if (absl::Status status =
            ValidateConsistentAttributes(last_old_real_input, first_new_input);
        !status.ok()) {
      return status;
    }
  }

  // If there are both new real and predicted inputs, check that the first
  // predicted input is valid against the last real input.
  if (!real_inputs.IsEmpty() && !predicted_inputs.IsEmpty()) {
    if (absl::Status status = ValidateConsistentAttributes(
            real_inputs.Last(), predicted_inputs.First());
        !status.ok()) {
      return status;
    }
  }

  return absl::OkStatus();
}

absl::Status InProgressStroke::ValidateNewElapsedTime(
    Duration32 current_elapsed_time) const {
  if (current_elapsed_time < Duration32::Zero()) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "Values of `current_elapsed_time` must be non-negative. Got %v.",
        current_elapsed_time));
  }

  if (current_elapsed_time < current_elapsed_time_) {
    return absl::InvalidArgumentError(
        absl::StrFormat("Values of `current_elapsed_time` must be "
                        "non-decreasing. Got %v followed by %v.",
                        current_elapsed_time_, current_elapsed_time));
  }

  return absl::OkStatus();
}

Stroke InProgressStroke::CopyToStroke(
    RetainAttributes retain_attributes) const {
  const Brush* brush = GetBrush();
  ABSL_CHECK(brush);

  uint32_t num_coats = BrushCoatCount();
  absl::InlinedVector<absl::InlinedVector<MeshFormat::AttributeId,
                                          StrokeVertex::kMaxAttributeCount>,
                      1>
      omit_attributes(num_coats);
  absl::InlinedVector<PartitionedMesh::MutableMeshGroup, 1> mesh_groups;
  mesh_groups.reserve(num_coats);
  absl::InlinedVector<StrokeVertex::CustomPackingArray, 1>
      custom_packing_arrays(num_coats);

  for (uint32_t coat_index = 0; coat_index < num_coats; ++coat_index) {
    const MeshFormat& format = GetMeshFormat(coat_index);
    switch (retain_attributes) {
      case RetainAttributes::kAll:
        break;
      case RetainAttributes::kUsedByThisBrush: {
        absl::flat_hash_set<MeshFormat::AttributeId> required_attributes;
        brush_internal::AddAttributeIdsRequiredByCoat(
            brush->GetFamily().GetCoats()[coat_index], required_attributes);
        for (MeshFormat::Attribute attribute : format.Attributes()) {
          if (!required_attributes.contains(attribute.id)) {
            omit_attributes[coat_index].push_back(attribute.id);
          }
        }
        break;
      }
    }

    custom_packing_arrays[coat_index] = StrokeVertex::MakeCustomPackingArray(
        format, omit_attributes[coat_index]);

    mesh_groups.push_back({
        .mesh = &GetMesh(coat_index),
        .outlines = GetCoatOutlines(coat_index),
        .omit_attributes = omit_attributes[coat_index],
        .packing_params = custom_packing_arrays[coat_index].Values(),
    });
  }
  absl::StatusOr<PartitionedMesh> partitioned_mesh =
      PartitionedMesh::FromMutableMeshGroups(mesh_groups);
  if (partitioned_mesh.ok()) {
    return Stroke(*brush, processed_inputs_.MakeDeepCopy(), *partitioned_mesh);
  } else {
    ABSL_LOG(WARNING)
        << "Failed to create PartitionedMesh for InProgressStroke: "
        << partitioned_mesh.status();
    return Stroke(*brush, processed_inputs_.MakeDeepCopy(),
                  PartitionedMesh::WithEmptyGroups(brush->CoatCount()));
  }
}

}  // namespace ink
