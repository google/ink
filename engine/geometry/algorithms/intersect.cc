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

#include "ink/engine/geometry/algorithms/intersect.h"

#include <cmath>
#include <iterator>

#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/floats.h"

namespace ink {
namespace geometry {
namespace {

// Returns an exponent X such that we can compute the intersection of
// 2^X * segment1 and 2^X * segment2 without causing an overflow error.
int FindSafeExponentForSegmentIntersection(const Segment &segment1,
                                           const Segment &segment2) {
  using util::floats::Exponent;
  using util::floats::MaxExponent;
  using util::floats::MinExponent;

  // This approximation allows us to quickly determine that a segment pair will
  // not cause an overflow or underflow error. The derivation is as follows:
  //
  // The determinant of the segment vectors, where the overflow/underflow will
  // occur, is of the form (a - b)(c - d) - (e - f)(g - h). Overflow and
  // underflow errors occur in the following cases, where F_min and F_max are
  // the minimum and maximum positive float values:
  // - Any of |a - b|, |c - d|, |e - f|, or |g - h| is greater than F_max
  // - Either of |(a - b)(c - d)| or |(e - f)(g - h)| is greater than F_max
  // - Both of |(a - b)(c - d)| and |(e - f)(g - h)| are less than F_min (if
  //   only one is less than F_min, the underflow still occurs, but it does not
  //   cause an error because the other term is sufficiently large)
  // - The absolute value of the determinant, |(a - b)(c - d) - (e - f)(g - h)|,
  //   is greater than F_max
  // If we let m be the maximum absolute value of the segment endpoint's
  // components (i.e. m = max(|a|, |b|, |c|, |d|, |e|, |f|, |g|, |h|)), then we
  // can see that the following are true:
  // - 2m ≥ max(|a - b|, |c - d|, |e - f|, |g - h|)
  // - 4m² ≥ max(|(a - b)(c - d)|, |(e - f)(g - h)|)
  // - 8m² ≥ max(|(a - b)(c - d) - (e - f)(g - h)|)
  // We can then see that an overflow or underflow error can only occur if at
  // least on of the following are true:
  // - 2m ≥ F_max
  // - 4m² ≥ F_max
  // - 4m² ≤ F_min
  // - 8m² ≥ F_max
  // Therefore, if m lies in the interval [sqrt(F_min/4), sqrt(F_max/8)], then
  // no overflow or underflow error can occur.
  static const float kSafeUpperBound =
      std::sqrt(.125f * std::numeric_limits<float>::max());
  static const float kSafeLowerBound =
      std::sqrt(.25f * std::numeric_limits<float>::min());
  float max_abs_value = std::max(
      std::max(std::max(std::abs(segment1.from.x), std::abs(segment1.from.y)),
               std::max(std::abs(segment1.to.x), std::abs(segment1.to.y))),
      std::max(std::max(std::abs(segment2.from.x), std::abs(segment2.from.y)),
               std::max(std::abs(segment2.to.x), std::abs(segment2.to.y))));
  if (max_abs_value >= kSafeLowerBound && max_abs_value <= kSafeUpperBound)
    return 0;

  // The most egregious losses of precision occur either during multiplication
  // or division, however there are some edge cases where you may get an
  // overflow from adding or subtracting.
  // When adding two positive numbers (or subtracting two negative numbers), the
  // exponent of the result can only be as large as the largest exponent plus 1.
  // Fortunately, we don't have to consider underflows (going past the minimum
  // exponent) for addition and subtraction.
  int s1xExp =
      Exponent(std::max(std::abs(segment1.to.x), std::abs(segment1.from.x))) +
      1;
  int s1yExp =
      Exponent(std::max(std::abs(segment1.to.y), std::abs(segment1.from.y))) +
      1;
  int s2xExp =
      Exponent(std::max(std::abs(segment2.to.x), std::abs(segment2.from.x))) +
      1;
  int s2yExp =
      Exponent(std::max(std::abs(segment2.to.y), std::abs(segment2.from.y))) +
      1;

  int maxPrecisionExponent = MaxExponent<float>();
  int minPrecisionExponent = MinExponent<float>();

  // The maximum exponent of a multiplication operation can only be as large as
  // the exponents of its operands added together.
  int expectedDetExponent = std::max(s2xExp + s1yExp, s1xExp + s2yExp) + 1;
  int expectedLenSquaredExponent =
      2 * std::max(std::max(s1xExp, s1yExp), std::max(s2xExp, s2yExp)) + 1;
  int expectedMaxExponent =
      std::max(expectedDetExponent, expectedLenSquaredExponent);

  int shiftExponent = 0;
  if (expectedMaxExponent > maxPrecisionExponent) {
    shiftExponent -= expectedMaxExponent - maxPrecisionExponent;
  } else if (expectedMaxExponent <= minPrecisionExponent) {
    shiftExponent -= expectedMaxExponent - minPrecisionExponent;
  }

  return shiftExponent;
}

bool SegmentIntersectionHelper(Segment segment1, Segment segment2,
                               std::array<float, 2> *seg1_interval,
                               std::array<float, 2> *seg2_interval) {
  if (segment1.from == segment2.from && segment1.to == segment2.to) {
    *seg1_interval = {{0, 1}};
    *seg2_interval = {{0, 1}};
    return true;
  } else if (segment1.from == segment2.to && segment1.to == segment2.from) {
    *seg1_interval = {{0, 1}};
    *seg2_interval = {{1, 0}};
    return true;
  }

  int exponent = FindSafeExponentForSegmentIntersection(segment1, segment2);
  if (exponent != 0) {
    segment1.from.x = ldexp(segment1.from.x, exponent);
    segment1.from.y = ldexp(segment1.from.y, exponent);
    segment1.to.x = ldexp(segment1.to.x, exponent);
    segment1.to.y = ldexp(segment1.to.y, exponent);
    segment2.from.x = ldexp(segment2.from.x, exponent);
    segment2.from.y = ldexp(segment2.from.y, exponent);
    segment2.to.x = ldexp(segment2.to.x, exponent);
    segment2.to.y = ldexp(segment2.to.y, exponent);
  }

  glm::vec2 u = segment1.to - segment1.from;
  glm::vec2 v = segment2.to - segment2.from;
  glm::vec2 w = segment2.from - segment1.from;
  float u_len_squared = glm::dot(u, u);
  float v_len_squared = glm::dot(v, v);

  if (u_len_squared == 0 && v_len_squared == 0) {
    // Both segments are degenerate -- they intersect only if they are also
    // coincident.
    if (glm::dot(w, w) == 0) {
      *seg1_interval = {{0, 1}};
      *seg2_interval = {{0, 1}};
      return true;
    } else {
      return false;
    }
  }

  bool segment1_start_is_collinear_with_segment2 =
      Orientation(segment2.from, segment2.to, segment1.from) ==
      RelativePos::kCollinear;
  bool segment1_end_is_collinear_with_segment2 =
      Orientation(segment2.from, segment2.to, segment1.to) ==
      RelativePos::kCollinear;
  bool segment2_start_is_collinear_with_segment1 =
      Orientation(segment1.from, segment1.to, segment2.from) ==
      RelativePos::kCollinear;
  bool segment2_end_is_collinear_with_segment1 =
      Orientation(segment1.from, segment1.to, segment2.to) ==
      RelativePos::kCollinear;

  glm::vec2 v_plus_w = segment2.to - segment1.from;   // v + w;
  glm::vec2 u_minus_w = segment1.to - segment2.from;  // u - w;

  // Check if the segments are parallel.
  if ((segment1_start_is_collinear_with_segment2 &&
       segment1_end_is_collinear_with_segment2) ||
      (segment2_start_is_collinear_with_segment1 &&
       segment2_end_is_collinear_with_segment1)) {
    (*seg1_interval)[0] = glm::dot(u, w) / u_len_squared;
    (*seg1_interval)[1] = glm::dot(u, v_plus_w) / u_len_squared;
    bool segments_travel_in_opposite_directions =
        (*seg1_interval)[1] < (*seg1_interval)[0];
    if (segments_travel_in_opposite_directions)
      std::swap((*seg1_interval)[0], (*seg1_interval)[1]);
    if ((*seg1_interval)[1] < 0 || (*seg1_interval)[0] > 1) {
      return false;
    } else if ((*seg1_interval)[0] < 0) {
      (*seg1_interval)[0] = 0;
      (*seg2_interval)[0] = glm::dot(v, -w) / v_len_squared;
    } else {
      (*seg2_interval)[0] = segments_travel_in_opposite_directions ? 1 : 0;
    }
    if ((*seg1_interval)[1] > 1) {
      (*seg1_interval)[1] = 1;
      (*seg2_interval)[1] = glm::dot(v, u_minus_w) / v_len_squared;
    } else {
      (*seg2_interval)[1] = segments_travel_in_opposite_directions ? 0 : 1;
    }
    return true;
  }

  auto populate_params = [](float segment1_param, float segment2_param,
                            std::array<float, 2> *seg1_interval,
                            std::array<float, 2> *seg2_interval) {
    (*seg1_interval)[0] = segment1_param;
    (*seg1_interval)[1] = segment1_param;
    (*seg2_interval)[0] = segment2_param;
    (*seg2_interval)[1] = segment2_param;
  };

  if (segment1_start_is_collinear_with_segment2) {
    // The start of segment1 is collinear with segment2.
    float p = glm::dot(v, -w) / v_len_squared;
    if (0 <= p && p <= 1) {
      populate_params(0, p, seg1_interval, seg2_interval);
      return true;
    }
  }
  if (segment1_end_is_collinear_with_segment2) {
    // The end of segment1 is collinear with segment2.
    float p = glm::dot(v, u_minus_w) / v_len_squared;
    if (0 <= p && p <= 1) {
      populate_params(1, p, seg1_interval, seg2_interval);
      return true;
    }
  }
  if (segment2_start_is_collinear_with_segment1) {
    // The start of segment2 is collinear with segment1.
    float p = glm::dot(u, w) / u_len_squared;
    if (0 <= p && p <= 1) {
      populate_params(p, 0, seg1_interval, seg2_interval);
      return true;
    }
  }
  if (segment2_end_is_collinear_with_segment1) {
    // The end of segment2 is collinear with segment1.
    float p = glm::dot(u, v_plus_w) / u_len_squared;
    if (0 <= p && p <= 1) {
      populate_params(p, 1, seg1_interval, seg2_interval);
      return true;
    }
  }

  float determinant = Determinant(u, v);
  if (determinant == 0) {
    // While we already checked for parallel segments, the determinant could
    // still be zero if one of the segments is degenerate.
    return false;
  }
  float segment1_param = Determinant(w, v) / determinant;
  float segment2_param = Determinant(w, u) / determinant;

  if (0 <= segment1_param && segment1_param <= 1 && 0 <= segment2_param &&
      segment2_param <= 1) {
    populate_params(segment1_param, segment2_param, seg1_interval,
                    seg2_interval);
    return true;
  } else {
    return false;
  }
}

}  // namespace

bool Intersects(const Segment &segment1, const Segment &segment2) {
  std::array<float, 2> placeholder1;
  std::array<float, 2> placeholder2;
  return SegmentIntersectionHelper(segment1, segment2, &placeholder1,
                                   &placeholder2);
}

bool Intersection(const Segment &segment1, const Segment &segment2,
                  SegmentIntersection *output) {
  bool intersection_found =
      SegmentIntersectionHelper(segment1, segment2, &output->segment1_interval,
                                &output->segment2_interval);
  if (intersection_found) {
    output->intx.from = segment1.Eval(output->segment1_interval[0]);
    output->intx.to = segment1.Eval(output->segment1_interval[1]);
  }
  return intersection_found;
}

bool Intersection(const Segment &segment1, const Segment &segment2,
                  glm::vec2 *output) {
  SegmentIntersection intx;
  bool retval = Intersection(segment1, segment2, &intx);
  if (retval) *output = intx.intx.from;
  return retval;
}

bool Intersects(const Triangle &triangle1, const Triangle &triangle2) {
  for (size_t i = 0; i < 3; ++i) {
    Segment segment1(triangle1[i], i == 2 ? triangle1[0] : triangle1[i + 1]);
    for (size_t j = 0; j < 3; ++j) {
      Segment segment2(triangle2[j], j == 2 ? triangle2[0] : triangle2[j + 1]);
      if (Intersects(segment1, segment2)) return true;
    }
  }

  return triangle1.Contains(triangle2[0]) || triangle2.Contains(triangle1[0]);
}

bool Intersects(const Rect &rect1, const Rect &rect2) {
  return std::max(rect1.from.x, rect2.from.x) <=
             std::min(rect1.to.x, rect2.to.x) &&
         std::max(rect1.from.y, rect2.from.y) <=
             std::min(rect1.to.y, rect2.to.y);
}

bool IntersectsWithNonZeroOverlap(const Rect &rect1, const Rect &rect2) {
  return std::max(rect1.from.x, rect2.from.x) <
             std::min(rect1.to.x, rect2.to.x) &&
         std::max(rect1.from.y, rect2.from.y) <
             std::min(rect1.to.y, rect2.to.y) &&
         !rect1.Empty() && !rect2.Empty();
}

bool Intersection(const Rect &rect1, const Rect &rect2, Rect *output) {
  output->from.x = std::max(rect1.from.x, rect2.from.x);
  output->from.y = std::max(rect1.from.y, rect2.from.y);
  output->to.x = std::min(rect1.to.x, rect2.to.x);
  output->to.y = std::min(rect1.to.y, rect2.to.y);

  if (output->from.x <= output->to.x && output->from.y <= output->to.y) {
    return true;
  } else {
    *output = Rect::CreateAtPoint(output->Center(), 0, 0);
    return false;
  }
}

bool Intersects(const RotRect &rect1, const RotRect &rect2) {
  auto corners = rect1.Corners();
  Triangle rect1_tri1(corners[0], corners[1], corners[2]);
  Triangle rect1_tri2(corners[0], corners[3], corners[2]);

  corners = rect2.Corners();
  Triangle rect2_tri1(corners[0], corners[1], corners[2]);
  Triangle rect2_tri2(corners[0], corners[3], corners[2]);

  return Intersects(rect1_tri1, rect2_tri1) ||
         Intersects(rect1_tri2, rect2_tri1) ||
         Intersects(rect1_tri1, rect2_tri2) ||
         Intersects(rect1_tri2, rect2_tri2);
}

bool Intersects(const Segment &segment, const Rect &rect) {
  if (rect.Contains(segment.from) && rect.Contains(segment.to)) {
    return true;
  }
  for (ink::Segment rect_segment : {rect.LeftSegment(), rect.RightSegment(),
                                    rect.BottomSegment(), rect.TopSegment()}) {
    if (Intersects(rect_segment, segment)) {
      return true;
    }
  }
  return false;
}

bool Intersection(const Segment &segment, const Rect &rect, Segment *output) {
  if (rect.Contains(segment.from) && rect.Contains(segment.to)) {
    *output = segment;
    return true;
  }
  std::vector<glm::vec2> intersections;
  for (ink::Segment rect_segment : {rect.LeftSegment(), rect.RightSegment(),
                                    rect.BottomSegment(), rect.TopSegment()}) {
    glm::vec2 intersection{0, 0};
    if (Intersection(rect_segment, segment, &intersection)) {
      if (std::find(intersections.begin(), intersections.end(), intersection) ==
          intersections.end()) {
        intersections.push_back(intersection);
      }
    }
  }
  if (intersections.empty()) {
    return false;
  }
  // We have an intersection. There may be only one point, if we have one of the
  // points of the segment in the rectangle, or two points, if both segments are
  // outside the rect.
  if (intersections.size() == 1) {
    *output = segment;
    // One point of the segment is in the rect, and the other is not.
    if (rect.Contains(segment.from)) {
      output->to = intersections[0];
    } else {
      output->from = intersections[0];
    }
  } else {
    output->from = intersections[0];
    output->to = intersections[1];
  }
  return true;
}

bool Intersection(const Polygon &polygon1, const Polygon &polygon2,
                  std::vector<PolygonIntersection> *output) {
  EXPECT(output->empty());
  for (int idx1 = 0; idx1 < polygon1.Size(); ++idx1) {
    for (int idx2 = 0; idx2 < polygon2.Size(); ++idx2) {
      SegmentIntersection intersection;
      if (Intersection(polygon1.GetSegment(idx1), polygon2.GetSegment(idx2),
                       &intersection)) {
        output->emplace_back();
        output->back().indices = {{idx1, idx2}};
        output->back().intersection = intersection;
      }
    }
  }

  return !output->empty();
}

}  // namespace geometry
}  // namespace ink
