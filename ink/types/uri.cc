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

#include "ink/types/uri.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"

namespace ink {

namespace {

constexpr int32_t kDefaultRevisionNumber = 1;

bool IsLowerOrHyphen(absl::string_view s) {
  return absl::c_all_of(
      s, [](char c) { return absl::ascii_islower(c) || c == '-'; });
}

bool IsDigit(absl::string_view s) {
  return absl::c_all_of(s, [](char c) { return absl::ascii_isdigit(c); });
}

size_t DistanceToNextDelimeterOrEnd(
    absl::string_view complete_uri, size_t position,
    absl::string_view possible_trailing_delimeters) {
  return std::min(
             complete_uri.find_first_of(possible_trailing_delimeters, position),
             complete_uri.length()) -
         position;
}

size_t FirstPositionAfterScheme(absl::string_view complete_uri) {
  constexpr absl::string_view kInkScheme = "ink:";

  return absl::StartsWithIgnoreCase(complete_uri, kInkScheme)
             ? kInkScheme.length()
             : 0;
}

}  // namespace

namespace uri_internal {

std::string ToFormattedString(Uri::AssetType asset_type) {
  switch (asset_type) {
    case Uri::AssetType::kUninitialized:
      return "uninitialized";
    case Uri::AssetType::kBrushFamily:
      return "brush-family";
  }
  return absl::StrCat("AssetType(", static_cast<int>(asset_type), ")");
}

}  // namespace uri_internal

absl::StatusOr<Uri::PartRange> Uri::ParseRegNameIfPresent(
    absl::string_view complete_uri, size_t& position) {
  constexpr absl::string_view kRegLeadingDelimeter = "//";
  constexpr absl::string_view kRegPossibleTrailingDelimeter = "/?#";

  if (!absl::StartsWith(complete_uri.substr(position), kRegLeadingDelimeter)) {
    return PartRange{};
  }
  position += kRegLeadingDelimeter.length();

  PartRange range = {
      .start = position,
      .length = DistanceToNextDelimeterOrEnd(complete_uri, position,
                                             kRegPossibleTrailingDelimeter)};
  position += range.length;

  absl::string_view reg_name = complete_uri.substr(range.start, range.length);
  if (reg_name.empty()) {
    return absl::InvalidArgumentError(
        absl::Substitute("Invalid uri: Non-empty reg-name is required if "
                         "delimiter $0 is present. Got: $1",
                         kRegLeadingDelimeter, complete_uri));
  }
  if (!IsLowerOrHyphen(reg_name)) {
    return absl::InvalidArgumentError(
        absl::Substitute("Invalid uri: Reg-name must consist of lower case "
                         "alpha or hyphen. Got: $0",
                         reg_name));
  }

  return range;
}

absl::StatusOr<Uri::PartRange> Uri::ParseAssetType(
    absl::string_view complete_uri, size_t& position) {
  constexpr absl::string_view kAssetTypeLeadingDelimeter = "/";
  constexpr absl::string_view kAssetTypePossibleTrailingDelimeter = "/:?#";

  if (!absl::StartsWith(complete_uri.substr(position),
                        kAssetTypeLeadingDelimeter)) {
    return absl::InvalidArgumentError(
        absl::Substitute("Invalid uri: Expected non-empty asset-type at "
                         "position $0 starting with $1.",
                         position, kAssetTypeLeadingDelimeter));
  }
  position += kAssetTypeLeadingDelimeter.length();

  PartRange range = {
      .start = position,
      .length = DistanceToNextDelimeterOrEnd(
          complete_uri, position, kAssetTypePossibleTrailingDelimeter)};
  position += range.length;

  absl::string_view asset_type = complete_uri.substr(range.start, range.length);
  if (asset_type.empty()) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Non-empty asset-type is required. Got: $0",
        complete_uri));
  }
  if (!IsLowerOrHyphen(asset_type)) {
    return absl::InvalidArgumentError(
        absl::Substitute("Invalid uri: Asset-type must consist of lower case "
                         "alpha or hyphen. Got: $0",
                         asset_type));
  }

  return range;
}

absl::StatusOr<Uri::PartRange> Uri::ParseAssetName(
    absl::string_view complete_uri, size_t& position) {
  if (complete_uri.substr(position).empty()) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Non-empty asset-name is required at position $0. Got: $1",
        position, complete_uri));
  }
  constexpr absl::string_view kAssetNameLeadingDelimeter = ":";
  constexpr absl::string_view kAssetNamePossibleTrailingDelimeter = "/:?#";

  if (!absl::StartsWith(complete_uri.substr(position),
                        kAssetNameLeadingDelimeter)) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Asset-name must start with $0. Got: $1 at position $2",
        kAssetNameLeadingDelimeter, complete_uri[position], position));
  }
  position += kAssetNameLeadingDelimeter.length();

  PartRange range = {
      .start = position,
      .length = DistanceToNextDelimeterOrEnd(
          complete_uri, position, kAssetNamePossibleTrailingDelimeter)};
  position += range.length;

  absl::string_view asset_name = complete_uri.substr(range.start, range.length);
  if (asset_name.empty()) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Non-empty asset-name is required at position $0. Got: $1",
        position, complete_uri));
  }
  if (!IsLowerOrHyphen(asset_name)) {
    return absl::InvalidArgumentError(
        absl::Substitute("Invalid uri: Asset-name must consist of lower case "
                         "alpha or hyphen. Got: $0",
                         asset_name));
  }

  return range;
}

absl::StatusOr<Uri::PartRange> Uri::ParseRevisionIfPresent(
    absl::string_view complete_uri, size_t& position) {
  constexpr absl::string_view kRevisionLeadingDelimeter = ":";
  constexpr absl::string_view kRevisionPossibleTrailingDelimeter = "/:?#";

  if (!absl::StartsWith(complete_uri.substr(position),
                        kRevisionLeadingDelimeter)) {
    return PartRange{};
  }
  position += kRevisionLeadingDelimeter.length();

  PartRange range = {
      .start = position,
      .length = DistanceToNextDelimeterOrEnd(
          complete_uri, position, kRevisionPossibleTrailingDelimeter)};
  position += range.length;

  absl::string_view revision = complete_uri.substr(range.start, range.length);
  if (revision.empty()) {
    return absl::InvalidArgumentError(
        absl::Substitute("Invalid uri: Revision is required if delimiter $0 "
                         "is present. Got: $1",
                         kRevisionLeadingDelimeter, complete_uri));
  }
  if (!IsDigit(revision)) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Revision must consist of all digits. Got: $0", revision));
  }
  if (revision.front() == '0') {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Revision must not have leading zero. Got: $0", revision));
  }
  // We limit to nine digits to ensure that the revision number fits in a signed
  // 32-bit int (which has a max value of 2,147,483,647). Limiting the number of
  // digits is simpler to specify and validate than limiting the parsed value.
  if (revision.size() > 9) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Revision must contain at most nine digits. Got: $0",
        revision));
  }

  return range;
}

absl::StatusOr<Uri> Uri::Parse(absl::string_view uri) {
  Uri validated_uri;
  if (uri.empty()) return validated_uri;

  size_t position = FirstPositionAfterScheme(uri);

  if (uri.substr(position).empty()) {
    return absl::InvalidArgumentError(
        "Invalid uri: Asset-type and asset-name are required "
        "but only scheme provided");
  }

  if (uri[position] != '/') {
    return absl::InvalidArgumentError(
        "Invalid uri: Non-scheme part of uri must start with /");
  }

  absl::StatusOr<PartRange> range;

  range = ParseRegNameIfPresent(uri, position);
  if (!range.ok()) return range.status();
  validated_uri.reg_name_ = uri.substr(range->start, range->length);
  if (validated_uri.reg_name_.empty()) {
    validated_uri.reg_name_ = InkRegName();
  }

  range = ParseAssetType(uri, position);
  if (!range.ok()) return range.status();
  absl::string_view asset_type_string =
      absl::string_view(uri).substr(range->start, range->length);

  range = ParseAssetName(uri, position);
  if (!range.ok()) return range.status();
  validated_uri.asset_name_ = uri.substr(range->start, range->length);

  range = ParseRevisionIfPresent(uri, position);
  if (!range.ok()) return range.status();
  int32_t revision_number;
  if (!absl::SimpleAtoi(uri.substr(range->start, range->length),
                        &revision_number)) {
    revision_number = kDefaultRevisionNumber;
  }
  validated_uri.revision_number_ = revision_number;

  if (position != uri.length()) {
    return absl::InvalidArgumentError(
        absl::Substitute("Invalid uri: Expected end of string but found: $0. "
                         "Fragment and query are not supported.",
                         uri.substr(position)));
  }

  if (asset_type_string == "brush-family") {
    validated_uri.asset_type_ = AssetType::kBrushFamily;
  } else {
    return absl::InvalidArgumentError(absl::Substitute(
        "Invalid uri: Invalid asset-type: '$0'", asset_type_string));
  }

  return validated_uri;
}

std::string Uri::ToNormalizedString() const {
  std::string uri_string;
  if (asset_type_ == AssetType::kUninitialized) return uri_string;

  if (!HasInkRegName()) {
    absl::StrAppend(&uri_string, "//", reg_name_);
  }

  absl::StrAppend(&uri_string, "/", asset_type_, ":", asset_name_);

  if (revision_number_ != kDefaultRevisionNumber) {
    absl::StrAppend(&uri_string, ":", revision_number_);
  }

  return uri_string;
}

}  // namespace ink
