/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ink/engine/rendering/page_tile_spec.h"

namespace ink {

using absl::string_view;

StatusOr<PageTileSpec> PageTileSpec::Parse(string_view uri) {
  const auto slashes_loc = uri.find("://");
  if (slashes_loc == string_view::npos) {
    return ErrorStatus(
        StatusCode::INVALID_ARGUMENT,
        "expected a :// to separate prefix from page tile uri fragment <$0>",
        uri);
  }
  // skip the uri prefix
  uri = uri.substr(slashes_loc + 3);
  size_t ques = uri.find("?");
  if (ques == string_view::npos) {
    return ErrorStatus("expected a ? to mark an uri param in <$0>", uri);
  }
  int32_t page_number;
  if (!absl::SimpleAtoi(uri.substr(0, ques), &page_number)) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "expected an integer page number at <$0>",
                       uri.substr(0, ques));
  }
  uri = uri.substr(ques + 1);
  ZoomSpec zoom_spec;
  INK_RETURN_UNLESS(ZoomSpec::FromUri(uri, &zoom_spec));
  return PageTileSpec(page_number, zoom_spec);
}

uint32_t PageTileSpec::DistanceFrom(const PageTileSpec& other) const {
  uint32_t depth_distance =
      std::abs(zoom_spec_.Depth() - other.zoom_spec_.Depth());
  uint32_t page_distance = std::min(0xFFFF, std::abs(page_ - other.page_));
  uint32_t equality_distance = other.zoom_spec_ == zoom_spec_ ? 0 : 1;
  return (depth_distance << 24) | (page_distance << 8) | equality_distance;
}

std::string PageTileSpec::ToUriFragment() const {
  return absl::Substitute("$0?$1", page_, zoom_spec_.ToUriParam());
}

}  // namespace ink
