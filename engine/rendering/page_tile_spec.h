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
#ifndef INK_ENGINE_RENDERING_PAGE_TILE_SPEC_H_
#define INK_ENGINE_RENDERING_PAGE_TILE_SPEC_H_

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/public/types/status_or.h"
#include "ink/engine/rendering/zoom_spec.h"

namespace ink {

// From a URI in the form prefix://pp?zoom=zz, parses the integer page index pp
// and the ZoomSpec zz.
class PageTileSpec {
 public:
  PageTileSpec(int page, const ZoomSpec& spec)
      : page_(page), zoom_spec_(spec) {}

  // Useful for unit tests:
  explicit PageTileSpec(int page) : PageTileSpec(page, ZoomSpec()) {}
  PageTileSpec(int page, Quadrant q)
      : PageTileSpec(page, ZoomSpec().ZoomedInto(q)) {}
  PageTileSpec(int page, Quadrant q, Quadrant r)
      : PageTileSpec(page, ZoomSpec().ZoomedInto(q).ZoomedInto(r)) {}
  PageTileSpec(int page, Quadrant q, Quadrant r, Quadrant s)
      : PageTileSpec(page,
                     ZoomSpec().ZoomedInto(q).ZoomedInto(r).ZoomedInto(s)) {}

  // Attempts to parse the given URI as prefix://pp?zoom=zz, ignoring the
  // prefix, and interpreting the rest as a page index and ZoomSpec.
  static StatusOr<PageTileSpec> Parse(absl::string_view uri);

  // Gives the URI fragment representation of this object, suitable for parsing
  // by the Parse function, when pasted onto some prefix://.
  std::string ToUriFragment() const;

  int Page() const { return page_; }
  const ZoomSpec& Zoom() const { return zoom_spec_; }

  // Returns a distance metric useful for sorting tiles by distance from the
  // given fragment, as when evicting tiles "farther" from a recently requested
  // tile.
  // The distance's most significant 8 bits are the difference between zoom
  // depths. This favors panning at a given zoom, over zooming at a given pan.
  // The next 16 bits are page distance.
  // The least significant bits are either 0 for the same zoom or 1 for a
  // different zoom. (That means any tile adjacent to this tile has the same
  // unitary distance, while an equivalent tile has 0 distance.)
  uint32_t DistanceFrom(const PageTileSpec& other) const;

  std::string ToString() const {
    return Substitute("<page $0 zoom $1>", page_, zoom_spec_);
  }

  inline PageTileSpec(const PageTileSpec& s) = default;
  inline PageTileSpec& operator=(const PageTileSpec& x) = default;
  inline bool operator==(const PageTileSpec& x) const {
    return x.page_ == page_ && x.zoom_spec_ == zoom_spec_;
  }
  inline bool operator!=(const PageTileSpec& x) const { return !(x == *this); }

 private:
  int page_;
  ZoomSpec zoom_spec_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_PAGE_TILE_SPEC_H_
