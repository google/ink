// Copyright 2018 Google LLC
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

#include "ink/engine/rendering/zoom_spec.h"

#include <algorithm>
#include <cstdlib>  // strtol

#include "third_party/absl/strings/numbers.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

Rect ZoomSpec::Apply(const Rect& target) const {
  Rect r = target.ContainingRectWithAspectRatio(1);
  for (const Quadrant q : ops_) {
    switch (q) {
      case Quadrant::NW:
        r = Rect(r.Left(), r.Center().y, r.Center().x, r.Top());
        break;
      case Quadrant::NE:
        r = Rect(r.Center().x, r.Center().y, r.Right(), r.Top());
        break;
      case Quadrant::SE:
        r = Rect(r.Center().x, r.Bottom(), r.Right(), r.Center().y);
        break;
      case Quadrant::SW:
        r = Rect(r.Left(), r.Bottom(), r.Center().x, r.Center().y);
        break;
    }
  }
  return r;
}

uint32_t ZoomSpec::Encode() const {
  uint32_t result = 0;
  for (int i = 0; i < ops_.size(); i++) {
    result |= (static_cast<uint8_t>(ops_.at(i)) << (3 * i));
  }
  return result;
}

Status ZoomSpec::Decode(uint32_t encoded, ZoomSpec* out) {
  uint32_t spec_mask = 0;
  std::vector<Quadrant> ops;
  for (int chunk_count = 0; chunk_count < 10; chunk_count++) {
    uint8_t chunk = (encoded >> (chunk_count * 3)) & 0b111;
    if (chunk == 0) break;
    if (chunk > 4) {
      return ErrorStatus(
          StatusCode::INTERNAL,
          "invalid zoomspec repr with non-quadrant bit pattern: $0", encoded);
    }
    ops.emplace_back(static_cast<Quadrant>(chunk));
    spec_mask |= (0b111 << (chunk_count * 3));
  }
  // We've hit a zero chunk. If there are any on-bits to the left of this chunk,
  // that's invalid.
  if (encoded & ~spec_mask) {
    return ErrorStatus(
        StatusCode::INTERNAL,
        Substitute("invalid zoomspec repr with non-zero bits to the left "
                   "of zero chunk: $0",
                   encoded));
  }
  *out = ZoomSpec(ops);
  return OkStatus();
}

std::string ZoomSpec::ToString() const {
  if (ops_.empty()) {
    return "FIT_ALL";
  }
  std::string result;
  for (const Quadrant q : ops_) {
    if (!result.empty()) {
      result.append("-");
    }
    switch (q) {
      case Quadrant::NW:
        result.append("NW");
        break;
      case Quadrant::NE:
        result.append("NE");
        break;
      case Quadrant::SE:
        result.append("SE");
        break;
      case Quadrant::SW:
        result.append("SW");
        break;
    }
  }
  return result;
}

ZoomSpec ZoomSpec::ZoomedInto(Quadrant quadrant) const {
  ZoomSpec r(*this);
  r.ops_.emplace_back(quadrant);
  return r;
}

static const char kUriParamKey[] = "zoom=";

std::string ZoomSpec::ToUriParam() const {
  return Substitute("$0$1", absl::string_view(kUriParamKey), Encode());
}

/* static */
Status ZoomSpec::FromUri(absl::string_view uri, ZoomSpec* out) {
  absl::string_view key(kUriParamKey);
  const auto& pos = uri.find(kUriParamKey);
  if (pos == absl::string_view::npos) {
    return ErrorStatus(StatusCode::INTERNAL,
                       Substitute("no $0 found in $1", key, uri));
  }
  // skip param chunk
  static const size_t param_key_length = key.size();
  uri = uri.substr(pos + param_key_length);
  const auto& end_pos = uri.find_first_not_of("0123456789");
  if (end_pos == 0) {
    return ErrorStatus(StatusCode::INTERNAL, "no encoded zoom spec found in $0",
                       uri);
  }
  uri = uri.substr(0, end_pos);
  uint32_t encoded_zoom_spec;
  if (!absl::SimpleAtoi(uri, &encoded_zoom_spec)) {
    return ErrorStatus(StatusCode::INTERNAL,
                       "could not interpret $0 as a uint32_t", uri);
  }
  return Decode(encoded_zoom_spec, out);
}

/* static */
bool ZoomSpec::HasZoomSpecParam(absl::string_view uri) {
  return uri.find(kUriParamKey) != absl::string_view::npos;
}

}  // namespace ink
