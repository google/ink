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

#ifndef INK_TYPES_URI_H_
#define INK_TYPES_URI_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace ink {

// `Uri' parses and validates a string containing a URI for an Ink asset.
//
// A valid Ink asset URI string is made up of the following parts:
//
// [<scheme>:][//<reg-name>]/<asset-type>:<asset-name>[:<revision>]
//
// with the following further restrictions:
//   * Parts enclosed in [] are optional, but must be non-empty if present.
//   * <scheme> must be "ink", and specifies a subset of the syntax laid out in
//     RFC 3986. As required by the spec, the scheme is case-insensitive, but
//     lower-case is preferred. If the scheme portion is omitted, it will be
//     interpreted as "ink".
//   * The registered name (<reg-name>) is the only allowed part of the
//     authority component of the URI. It must consist of lowercase alpha and
//     hyphen. The registered name "ink" is reserved for Ink-owned URIs (such
//     as built-in stock brushes), and if the registered name portion is
//     omitted, it will be interpreted as "ink" by default.
//   * The path component of the URI must consist of <asset-type>, <asset-name>,
//     and the optional <revision>:
//       * <asset-type> must be one of the following strings:
//           * "brush-family"
//           * "texture"
//       * <asset-name> must consist of lowercase alpha and hyphen.
//       * <revision> must be a positive base-10 integer with no leading zeroes
//         and at most nine digits (which guarantees that the value fits in a
//         32-bit int). If the revision portion is omitted, it is interpreted
//         as 1 by default (NOT "latest").
class Uri {
 public:
  // Asset-type values that an Ink asset URI can have.
  enum class AssetType : uint8_t {
    // Placeholder used for default-constructed `Uri` objects.
    kUninitialized,
    // Asset-type "brush-family", for identifying a specific `BrushFamily`.
    kBrushFamily,
    // Asset-type "texture", for identifying a specific
    // `color_texture_uri` for a `TextureLayer`.
    kTexture,
  };

  // Parses the provided `uri` into its parts.
  // Returns an error if the `uri` does not fulfill the requirements above.
  static absl::StatusOr<Uri> Parse(absl::string_view uri);

  // Returns the reg-name string for Ink-owned URIs. This is the default
  // reg-name that is assumed for URI strings that omit the reg-name.
  static absl::string_view InkRegName() { return "ink"; }

  Uri() = default;
  Uri(const Uri&) = default;
  Uri(Uri&&) = default;
  Uri& operator=(const Uri&) = default;
  Uri& operator=(Uri&&) = default;
  ~Uri() = default;

  bool operator==(const Uri& other) const;
  bool operator!=(const Uri& other) const { return !(*this == other); }

  // Returns the normalized string form of this URI. The returned string will be
  // as short as possible; in particular:
  //   * The scheme is omitted (since it can only be "ink").
  //   * If the reg-name is "ink", it is omitted (since that is the default).
  //   * If the revision is 1, it is omitted (since that is the default).
  //
  // If the `Uri` object was default-constructed, this will return the empty
  // string. Regardless, passing the returned string to `Uri::Parse` will
  // successfully return an equivalent `Uri`.
  std::string ToNormalizedString() const;

  // Returns true if this URI's reg-name is `InkRegName()`.
  bool HasInkRegName() const { return reg_name_ == InkRegName(); }

  // Returns the reg-name. If the reg-name was absent from the original string,
  // this returns the default value of `InkRegName()`. If the `Uri` was
  // default-constructed, this returns empty string.
  absl::string_view GetRegName() const { return reg_name_; }

  // Returns the asset-type. If the `Uri` was default-constructed, this returns
  // `AssetType::kUninitialized`.
  AssetType GetAssetType() const { return asset_type_; }

  // Returns the asset-name. If the `Uri` was default-constructed, this returns
  // empty string.
  absl::string_view GetAssetName() const { return asset_name_; }

  // Returns the revision number. If the revision number was absent from the
  // original string, this returns the default value of 1. If the `Uri` was
  // default-constructed, this returns 0.
  int32_t GetRevisionNumber() const { return revision_number_; }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, Uri uri) {
    sink.Append(uri.ToNormalizedString());
  }

  template <typename H>
  friend H AbslHashValue(H h, const Uri& uri);

 private:
  // Struct for holding start, relative to the entire `uri` string and length of
  // individual `uri` parts.
  struct PartRange {
    size_t start = 0;
    size_t length = 0;
  };

  // Parsers expect the full `uri` and the current position for the start of the
  // field.
  // Returns the PartRange for the element in question on success or an
  // InvalidArgumentError status if validation failed.
  static absl::StatusOr<PartRange> ParseRegNameIfPresent(
      absl::string_view complete_uri, size_t& position);
  static absl::StatusOr<PartRange> ParseAssetType(
      absl::string_view complete_uri, size_t& position);
  static absl::StatusOr<PartRange> ParseAssetName(
      absl::string_view complete_uri, size_t& position);
  static absl::StatusOr<PartRange> ParseRevisionIfPresent(
      absl::string_view complete_uri, size_t& position);

  // N.B. This field ordering minimizes padding bytes for field alignment.
  std::string reg_name_;
  std::string asset_name_;
  int32_t revision_number_ = 0;
  AssetType asset_type_ = AssetType::kUninitialized;
};

namespace uri_internal {
std::string ToFormattedString(Uri::AssetType asset_type);
}  // namespace uri_internal

template <typename Sink>
void AbslStringify(Sink& sink, Uri::AssetType asset_type) {
  sink.Append(uri_internal::ToFormattedString(asset_type));
}

template <typename H>
H AbslHashValue(H h, const Uri& uri) {
  return H::combine(std::move(h), uri.reg_name_, uri.asset_name_,
                    uri.revision_number_, uri.asset_type_);
}

inline bool Uri::operator==(const Uri& other) const {
  return asset_type_ == other.asset_type_ && reg_name_ == other.reg_name_ &&
         asset_name_ == other.asset_name_ &&
         revision_number_ == other.revision_number_;
}

}  // namespace ink

#endif  // INK_TYPES_URI_H_
