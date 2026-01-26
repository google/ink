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

#include "ink/brush/brush_family.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/types/duration.h"

namespace ink {

BrushFamily::InputModel BrushFamily::DefaultInputModel() {
  return SlidingWindowModel{};
}

uint32_t BrushFamily::MaxBrushCoats() {
  // This value was chosen somewhat arbitrarily. A `PartitionedMesh` can't have
  // more than 2^16 meshes, and each coat creates at least one mesh, so we need
  // *some* limit. We can always raise this limit in the future, but lowering it
  // will be harder once clients start relying on being able to have a certain
  // number of coats. So for now, the limit is fairly conservative.
  return 10;
}

BrushFamily::BrushFamily(absl::Span<const BrushCoat> coats,
                         const InputModel& input_model,
                         const Metadata& metadata)
    : coats_(coats.begin(), coats.end()),
      input_model_(input_model),
      metadata_(metadata) {}

absl::StatusOr<BrushFamily> BrushFamily::Create(const BrushTip& tip,
                                                const BrushPaint& paint,
                                                const InputModel& input_model,
                                                const Metadata& metadata) {
  BrushCoat coat = {.tip = tip, .paint_preferences = {paint}};
  return BrushFamily::Create(absl::MakeConstSpan(&coat, 1), input_model,
                             metadata);
}

absl::StatusOr<BrushFamily> BrushFamily::Create(
    absl::Span<const BrushCoat> coats, const InputModel& input_model,
    const Metadata& metadata) {
  if (coats.size() > MaxBrushCoats()) {
    return absl::InvalidArgumentError(
        absl::StrCat("A `BrushFamily` cannot have more than ", MaxBrushCoats(),
                     " `BrushCoat`s, but `coats.size()` was ", coats.size()));
  }
  for (const BrushCoat& coat : coats) {
    if (absl::Status status = brush_internal::ValidateBrushCoat(coat);
        !status.ok()) {
      return status;
    }
  }
  if (absl::Status status = brush_internal::ValidateInputModel(input_model);
      !status.ok()) {
    return status;
  }
  return BrushFamily(coats, input_model, metadata);
}

std::string BrushFamily::ToFormattedString() const {
  std::string formatted =
      absl::StrCat("BrushFamily(coats=[", absl::StrJoin(coats_, ", "),
                   "], input_model=", input_model_);
  if (!metadata_.client_brush_family_id.empty()) {
    absl::StrAppend(&formatted, ", client_brush_family_id='",
                    metadata_.client_brush_family_id, "'");
  }
  absl::StrAppend(&formatted, ")");
  return formatted;
}

namespace brush_internal {
namespace {

absl::Status ValidateInputModel(const BrushFamily::SpringModel& model) {
  return absl::OkStatus();
}

absl::Status ValidateInputModel(
    const BrushFamily::ExperimentalNaiveModel& model) {
  return absl::OkStatus();
}

absl::Status ValidateInputModel(const BrushFamily::SlidingWindowModel& model) {
  if (!model.window_size.IsFinite() ||
      model.window_size <= Duration32::Zero()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`SlidingWindowModel::window_size` must be finite and positive. Got: ",
        model.window_size));
  }
  if (model.upsampling_period <= Duration32::Zero()) {
    return absl::InvalidArgumentError(absl::StrCat(
        "`SlidingWindowModel::upsampling_period` must be positive. Got: ",
        model.upsampling_period));
  }
  return absl::OkStatus();
}

std::string ToFormattedString(const BrushFamily::SpringModel& model) {
  return "SpringModel";
}

std::string ToFormattedString(
    const BrushFamily::ExperimentalNaiveModel& model) {
  return "ExperimentalNaiveModel";
}

std::string ToFormattedString(const BrushFamily::SlidingWindowModel& model) {
  return absl::StrCat("SlidingWindowModel(window_size=", model.window_size,
                      ", upsampling_period=", model.upsampling_period, ")");
}

}  // namespace

absl::Status ValidateInputModel(const BrushFamily::InputModel& model) {
  return std::visit([](const auto& model) { return ValidateInputModel(model); },
                    model);
}

std::string ToFormattedString(const BrushFamily::InputModel& model) {
  return std::visit([](const auto& model) { return ToFormattedString(model); },
                    model);
}

}  // namespace brush_internal
}  // namespace ink
