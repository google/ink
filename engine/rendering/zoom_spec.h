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

#ifndef INK_ENGINE_RENDERING_ZOOM_SPEC_H_
#define INK_ENGINE_RENDERING_ZOOM_SPEC_H_

#include <vector>

#ifndef NDEBUG
#include <ostream>
#endif

#include "third_party/absl/strings/string_view.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/security.h"

namespace ink {

/*
 * A ZoomSpec describes a desired square area relative to any target rectangle.
 * Let's call the target rectangle R. R has dimensions W and H.
 *
 * An empty ZoomSpec means "the square with a side dimension of max(W, H), with
 * the smaller side of R centered". If R has dimensions 600x800, then
 * ZoomSpec 0 applied to R means the square (-100, 0)->(700, 800).
 *
 * A ZoomSpec is serialized as an unsigned 32 bit integer, interpreted in 3-bit
 * chunks, parsed from the least significant 3 bits towards the most
 * significant. As soon as we encounter a 3-bit chunk with no bits set, we stop
 * parsing the spec.
 *
 * There are 5 meaningful values, having 3-bit values 1, 2, 3, and 4,
 * corresponding respectively to NW, NE, SE, and SW quadrants, and 0, meaning
 * "stop parsing".
 *
 *     given zoomspec S and Rect R
 *     Z = square_containing_centered(R)
 *     chunk = least significant 3 bits of S
 *     while chunk is non-zero:
 *       switch(chunk):
 *         case NW: Z = (Z.left, Z.midy)->(Z.midx, Z.top)
 *         case NE: Z = (Z.midx, Z.midy)->(Z.right, Z.top)
 *         case SE: Z = (Z.midx, Z.bottom)->(Z.right, Z.midy)
 *         case SW: Z = (Z.left, Z.bottom)->(Z.midx, Z,midy)
 *       S = S >> 3
 *       chunk = least significant 3 bits of S
 *
 * Given that there's room for 10 zoom operations in a 32-bit word, one can
 * specify a zoom operation to blow up our example Rect R to 614400x819200,
 * which really ought to do it.
 */

enum class Quadrant : uint8_t { NW = 1, NE = 2, SE = 3, SW = 4 };
constexpr std::array<Quadrant, 4> kAllQuadrants = {
    {Quadrant::NW, Quadrant::NE, Quadrant::SE, Quadrant::SW}};

// ZoomSpec is immutable.
class ZoomSpec {
 public:
  ZoomSpec() {}
  ~ZoomSpec() {}

  // Apply this ZoomSpec to the given rectangle, and return the resulting
  // square.
  Rect Apply(const Rect& target) const;

  // Return a new ZoomSpec that would result from zooming into the given
  // quadrant of this.
  ZoomSpec ZoomedInto(Quadrant quadrant) const;

  // Returns a 32-bit value encoding this ZoomSpec for serialization.
  uint32_t Encode() const;

  // Returns true if this ZoomSpec is "empty", i.e., is zoomed all the way out.
  bool IsEmpty() const { return ops_.empty(); }

  // Attempt to interpret the given 32 bits as a ZoomSpec external form. If the
  // return value is ok(), the out param will contain a valid ZoomSpec.
  static S_WARN_UNUSED_RESULT Status Decode(uint32_t encoded, ZoomSpec* out);

  // Returns this zoom spec as an uri param/value string, e.g., zoom=12345
  std::string ToUriParam() const;

  // Find a zoom param key/value in the give uri, and populates the given
  // ZoomSpec pointer with the resulting spec. If a valid spec is found and
  // parsed, returns ok, otherwise returns an error.
  static S_WARN_UNUSED_RESULT Status FromUri(absl::string_view uri,
                                             ZoomSpec* out);

  // Returns true if the given uri appears to have a zoom spec parameter.
  static S_WARN_UNUSED_RESULT bool HasZoomSpecParam(absl::string_view uri);

  // A human-readable form, for debugging.
  std::string ToString() const;

  inline ZoomSpec(const ZoomSpec& s) = default;
  inline ZoomSpec& operator=(const ZoomSpec& x) = default;
  inline bool operator==(const ZoomSpec& x) const { return x.ops_ == ops_; }
  inline bool operator!=(const ZoomSpec& x) const { return !(x == *this); }

 private:
  explicit ZoomSpec(std::vector<Quadrant> ops) : ops_(ops) {}

  std::vector<Quadrant> ops_;

#ifndef NDEBUG
  // for unit test failures
  friend ::std::ostream& operator<<(::std::ostream& os, const ZoomSpec& spec) {
    return os << spec.ToString();
  }
#endif
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_ZOOM_SPEC_H_
