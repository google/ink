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
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/types/uri.h"

namespace ink {

BrushFamily::InputModel BrushFamily::DefaultInputModel() {
  return SpringModelV1{};
}

uint32_t BrushFamily::MaxBrushCoats() {
  // This value was chosen somewhat arbitrarily. A `PartitionedMesh` can't have
  // more than 2^16 meshes, and each coat creates at least one mesh, so we need
  // *some* limit. We can always raise this limit in the future, but lowering it
  // will be harder once clients start relying on being able to have a certain
  // number of coats. So for now, the limit is fairly conservative.
  return 10;
}

BrushFamily::BrushFamily(std::vector<BrushCoat> coats, std::optional<Uri> uri,
                         const InputModel& input_model)
    : coats_(std::move(coats)),
      uri_(std::move(uri)),
      input_model_(input_model) {}

absl::StatusOr<BrushFamily> BrushFamily::Create(
    absl::Span<const BrushCoat> coats, absl::string_view uri_string,
    const InputModel& input_model) {
  if (uri_string.empty()) {
    return BrushFamily::Create(coats, std::nullopt, input_model);
  }
  absl::StatusOr<Uri> uri = Uri::Parse(uri_string);
  if (!uri.ok()) {
    return uri.status();
  }
  return BrushFamily::Create(coats, *uri, input_model);
}

absl::StatusOr<BrushFamily> BrushFamily::Create(const BrushTip& tip,
                                                const BrushPaint& paint,
                                                absl::string_view uri_string,
                                                const InputModel& input_model) {
  BrushCoat coat = {.tips = {tip}, .paint = paint};
  return BrushFamily::Create(absl::MakeConstSpan(&coat, 1), uri_string,
                             input_model);
}

namespace {

absl::Status ValidateBrushUri(const Uri& uri) {
  if (uri.GetAssetType() != Uri::AssetType::kBrushFamily) {
    return absl::InvalidArgumentError(absl::StrFormat(
        "URI asset-type for a `BrushFamily` must be '%v'. Got '%v'",
        Uri::AssetType::kBrushFamily, uri.GetAssetType()));
  }
  return absl::OkStatus();
}

}  // namespace

absl::StatusOr<BrushFamily> BrushFamily::Create(
    absl::Span<const BrushCoat> coats, std::optional<Uri> uri,
    const InputModel& input_model) {
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
  if (uri.has_value()) {
    if (absl::Status status = ValidateBrushUri(*uri); !status.ok()) {
      return status;
    }
  }
  return BrushFamily(std::vector<BrushCoat>(coats.begin(), coats.end()),
                     std::move(uri), input_model);
}

absl::StatusOr<BrushFamily> BrushFamily::Create(const BrushTip& tip,
                                                const BrushPaint& paint,
                                                std::optional<Uri> uri,
                                                const InputModel& input_model) {
  BrushCoat coat = {.tips = {tip}, .paint = paint};
  return BrushFamily::Create(absl::MakeConstSpan(&coat, 1), std::move(uri),
                             input_model);
}

std::string BrushFamily::ToFormattedString() const {
  std::string formatted =
      absl::StrCat("BrushFamily(coats=[", absl::StrJoin(coats_, ", "), "]");
  if (uri_.has_value()) {
    absl::StrAppend(&formatted, ", uri='", *uri_, "'");
  }
  formatted.push_back(')');
  return formatted;
}

}  // namespace ink
