// Copyright 2024-2025 Google LLC
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

#include "ink/storage/brush_provider.h"

#include <cstdint>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/types/uri.h"

namespace ink {
namespace {

// TODO: b/293305476 - Add asset names for other stock brushes.
constexpr absl::string_view kMarkerAssetName = "marker";

// Returns the shortest-length URI that represents the stock Ink brush with the
// given asset name and revision number.
std::string CanonicalStockBrushFamilyUri(absl::string_view asset_name,
                                         int32_t revision) {
  std::string uri =
      absl::StrCat("/", Uri::AssetType::kBrushFamily, ":", asset_name);
  if (revision != 1) {
    absl::StrAppend(&uri, ":", revision);
  }
  return uri;
}

absl::Status InvalidStockBrushRevisionError(absl::string_view asset_name,
                                            int32_t revision) {
  return absl::NotFoundError(absl::StrCat("unknown stock '", asset_name,
                                          "' brush revision: ", revision));
}

absl::StatusOr<BrushFamily> GetStockMarkerBrush(int32_t revision) {
  constexpr absl::string_view kAssetName = kMarkerAssetName;
  if (revision != 1) {
    return InvalidStockBrushRevisionError(kAssetName, revision);
  }
  return BrushFamily::Create(BrushTip{.corner_rounding = 1.f}, BrushPaint{},
                             CanonicalStockBrushFamilyUri(kAssetName, 1));
}

absl::StatusOr<BrushFamily> GetStockBrushFamily(const Uri& uri) {
  absl::string_view asset_name = uri.GetAssetName();
  int32_t revision = uri.GetRevisionNumber();
  if (asset_name == kMarkerAssetName) {
    return GetStockMarkerBrush(revision);
  }
  // TODO: b/293305476 - Remove this once real stock brushes are implemented.
  if (asset_name == "inkpen" || asset_name == "pencil" ||
      asset_name == "highlighter" || asset_name == "charcoal") {
    return BrushFamily::Create(BrushTip{}, BrushPaint{},
                               CanonicalStockBrushFamilyUri(asset_name, 1));
  }
  // TODO: b/293305476 - Add else-if blocks for other stock brushes.
  return absl::NotFoundError(
      absl::StrCat("unknown stock brush asset name: ", asset_name));
}

}  // namespace

absl::StatusOr<BrushFamily> BrushProvider::GetBrushFamily(
    const Uri& uri) const {
  if (uri.GetAssetType() != Uri::AssetType::kBrushFamily) {
    return absl::InvalidArgumentError(
        absl::StrCat("not a brush family URI: ", uri));
  }
  if (!uri.HasInkRegName()) {
    return GetClientBrushFamily(uri);
  }
  return GetStockBrushFamily(uri);
}

absl::StatusOr<BrushFamily> BrushProvider::GetClientBrushFamily(
    const Uri& uri) const {
  return absl::NotFoundError(absl::StrCat("unknown brush family URI: ", uri));
}

}  // namespace ink
