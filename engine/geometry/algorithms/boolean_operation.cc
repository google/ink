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

#include "ink/engine/geometry/algorithms/boolean_operation.h"

#include <algorithm>
#include <array>
#include <iterator>

#include "third_party/absl/strings/str_join.h"
#include "third_party/absl/types/optional.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/primitives/segment.h"
#include "ink/engine/geometry/primitives/vector_utils.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/iterator/cyclic_iterator.h"

namespace ink {
namespace geometry {
namespace {

// This tolerance was chosen experimentally.
constexpr float kSnappingTol = 2 * std::numeric_limits<float>::epsilon();

// Returns true if the relative difference of both the x- and y-components of
// the given vectors are less than kSnappingTol.
bool RelativeErrorWithinSnappingTol(glm::vec2 a, glm::vec2 b) {
  return std::abs(a.x - b.x) <= std::abs(a.x) * kSnappingTol &&
         std::abs(a.y - b.y) <= std::abs(a.y) * kSnappingTol;
}

// This indicates the type of the vertex w.r.t. the traversal of its polygon,
// which may be either the left-hand-side or right-hand-side of the operation.
enum class VertexType {
  // A vertex from this polygon. It will not have a twin in the other traversal.
  kNonIntersection,
  // An intersection of a segment on this polygon with a segment or vertex on
  // the other polygon.
  kIntersection,
  // An intersection of a vertex on this polygon with a segment or vertex on the
  // other polygon.
  kIntersectionAtVertex
};

// Every intersection can be categorized as one of these types, determined by
// the arrangement of the previous and next vertices in the two traversals.
//
// In the diagrams below, "x" refers to the current vertex, "A0" and "A1" refer
// to the previous and next vertices in the current traversal, and "B0" and "B1"
// refer to the previous and next vertices in the other traversal.
//
// Given a vertex's intersection type, you can infer the intersection type of
// its twin (see TwinIntersectionType() below).
enum class IntersectionType {
  // Not an intersection, or not yet determined.
  kInvalid,
  //      ⇑ A1
  //      ⇑
  // B0 ⇒⇒x⇒⇒ B1
  //      ⇑
  //      ⇑ A0
  kCrossInsideToOutside,
  //      ⇑ A1
  //      ⇑
  // B1 ⇐⇐x⇐⇐ B0
  //      ⇑
  //      ⇑ A0
  kCrossOutsideToInside,
  //      ⇓ B0
  //      ⇓
  // B1 ⇐⇐x⇒⇒ A1
  //      ⇑
  //      ⇑ A0
  kTouchInsideToInside,
  //      ⇑ B1
  //      ⇑
  // B0 ⇒⇒x⇒⇒ A1
  //      ⇑
  //      ⇑ A0
  kTouchInsideToOutside,
  //      ⇑ B1
  //      ⇑
  // A1 ⇐⇐x⇐⇐ B0
  //      ⇑
  //      ⇑ A0
  kTouchOutsideToInside,
  //      ⇓ B0
  //      ⇓
  // A1 ⇐⇐x⇒⇒ B1
  //      ⇑
  //      ⇑ A0
  kTouchOutsideToOutside,
  //   A1 ⇑⇑ B1
  //      ⇑⇑
  // A0 ⇒⇒xx⇐⇐ B0
  kInsideToAlignedOverlap,
  //   B1 ⇑⇑ A1
  //      ⇑⇑
  // B0 ⇒⇒xx⇐⇐ A0
  kOutsideToAlignedOverlap,
  // A1 ⇐⇐xx⇒⇒ B1
  //      ⇑⇑
  //   A0 ⇑⇑ B0
  kAlignedOverlapToInside,
  // B1 ⇐⇐xx⇒⇒ A1
  //      ⇑⇑
  //   B0 ⇑⇑ A0
  kAlignedOverlapToOutside,
  // A0 ⇒⇒x⇒⇒ A1
  // B0 ⇒⇒x⇒⇒ B1
  kAlignedOverlapToAlignedOverlap,
  //   A1 ⇑⇓ B0
  //      ⇑⇓
  // A0 ⇒⇒xx⇒⇒ B1
  kInsideToReversedOverlap,
  //   B0 ⇓⇑ A1
  //      ⇓⇑
  // B1 ⇐⇐xx⇐⇐ A0
  kOutsideToReversedOverlap,
  // A1 ⇐⇐xx⇐⇐ B0
  //      ⇑⇓
  //   A0 ⇑⇓ B1
  kReversedOverlapToInside,
  // B0 ⇒⇒xx⇒⇒ A1
  //      ⇓⇑
  //   B1 ⇓⇑ A0
  kReversedOverlapToOutside,
  // A0 ⇒⇒x⇒⇒ A1
  // B1 ⇐⇐x⇐⇐ B0
  kReversedOverlapToReversedOverlap,
  // A1 ⇐⇐
  // B0 ⇒⇒x⇒⇒ B1
  // A0 ⇒⇒
  kAlignedOverlapToReversedOverlapCCW,
  // A0 ⇒⇒
  // B0 ⇒⇒x⇒⇒ B1
  // A1 ⇐⇐
  kAlignedOverlapToReversedOverlapCW,
  // A1 ⇐⇐
  // B1 ⇐⇐x⇐⇐ B0
  // A0 ⇒⇒
  kReversedOverlapToAlignedOverlapCCW,
  // A0 ⇒⇒
  // B1 ⇐⇐x⇐⇐ B0
  // A1 ⇐⇐
  kReversedOverlapToAlignedOverlapCW,
  // B1 ⇐⇐
  // A0 ⇒⇒x⇒⇒ A1
  // B0 ⇒⇒
  kSpikeOverlapToOutside,
  // B0 ⇒⇒
  // A0 ⇒⇒x⇒⇒ A1
  // B1 ⇐⇐
  kSpikeOverlapToInside,
  //       ⇐⇐ B0
  // A0 ⇒⇒x⇒⇒ A1
  //       ⇒⇒ B1
  kOutsideToSpikeOverlap,
  //       ⇒⇒ B1
  // A0 ⇒⇒x⇒⇒ A1
  //       ⇐⇐ B0
  kInsideToSpikeOverlap,
};

// Given a vertex's intersection type, returns the intersection type of its
// twin.
IntersectionType TwinIntersectionType(IntersectionType type) {
  switch (type) {
    case IntersectionType::kCrossInsideToOutside:
      return IntersectionType::kCrossOutsideToInside;
    case IntersectionType::kCrossOutsideToInside:
      return IntersectionType::kCrossInsideToOutside;
    case IntersectionType::kTouchInsideToOutside:
      return IntersectionType::kTouchOutsideToInside;
    case IntersectionType::kTouchOutsideToInside:
      return IntersectionType::kTouchInsideToOutside;
    case IntersectionType::kInsideToAlignedOverlap:
      return IntersectionType::kOutsideToAlignedOverlap;
    case IntersectionType::kOutsideToAlignedOverlap:
      return IntersectionType::kInsideToAlignedOverlap;
    case IntersectionType::kAlignedOverlapToInside:
      return IntersectionType::kAlignedOverlapToOutside;
    case IntersectionType::kAlignedOverlapToOutside:
      return IntersectionType::kAlignedOverlapToInside;
    case IntersectionType::kInsideToReversedOverlap:
      return IntersectionType::kReversedOverlapToInside;
    case IntersectionType::kReversedOverlapToInside:
      return IntersectionType::kInsideToReversedOverlap;
    case IntersectionType::kOutsideToReversedOverlap:
      return IntersectionType::kReversedOverlapToOutside;
    case IntersectionType::kReversedOverlapToOutside:
      return IntersectionType::kOutsideToReversedOverlap;
    case IntersectionType::kAlignedOverlapToReversedOverlapCCW:
      return IntersectionType::kSpikeOverlapToOutside;
    case IntersectionType::kSpikeOverlapToOutside:
      return IntersectionType::kAlignedOverlapToReversedOverlapCCW;
    case IntersectionType::kAlignedOverlapToReversedOverlapCW:
      return IntersectionType::kSpikeOverlapToInside;
    case IntersectionType::kSpikeOverlapToInside:
      return IntersectionType::kAlignedOverlapToReversedOverlapCW;
    case IntersectionType::kReversedOverlapToAlignedOverlapCCW:
      return IntersectionType::kOutsideToSpikeOverlap;
    case IntersectionType::kOutsideToSpikeOverlap:
      return IntersectionType::kReversedOverlapToAlignedOverlapCCW;
    case IntersectionType::kReversedOverlapToAlignedOverlapCW:
      return IntersectionType::kInsideToSpikeOverlap;
    case IntersectionType::kInsideToSpikeOverlap:
      return IntersectionType::kReversedOverlapToAlignedOverlapCW;

    // These types are their own twins.
    case IntersectionType::kInvalid:
    case IntersectionType::kTouchInsideToInside:
    case IntersectionType::kTouchOutsideToOutside:
    case IntersectionType::kAlignedOverlapToAlignedOverlap:
    case IntersectionType::kReversedOverlapToReversedOverlap:
      return type;
  }
}

bool IsOverlapToOverlapType(IntersectionType type) {
  return type == IntersectionType::kAlignedOverlapToAlignedOverlap ||
         type == IntersectionType::kReversedOverlapToReversedOverlap ||
         type == IntersectionType::kAlignedOverlapToReversedOverlapCCW ||
         type == IntersectionType::kAlignedOverlapToReversedOverlapCW ||
         type == IntersectionType::kReversedOverlapToAlignedOverlapCCW ||
         type == IntersectionType::kReversedOverlapToAlignedOverlapCW;
}

bool IsOverlapToNonOverlapType(IntersectionType type) {
  return type == IntersectionType::kAlignedOverlapToInside ||
         type == IntersectionType::kAlignedOverlapToOutside ||
         type == IntersectionType::kReversedOverlapToInside ||
         type == IntersectionType::kReversedOverlapToOutside ||
         type == IntersectionType::kSpikeOverlapToOutside ||
         type == IntersectionType::kSpikeOverlapToInside;
}

bool IsNonOverlapToOverlapType(IntersectionType type) {
  return type == IntersectionType::kInsideToAlignedOverlap ||
         type == IntersectionType::kOutsideToAlignedOverlap ||
         type == IntersectionType::kInsideToReversedOverlap ||
         type == IntersectionType::kOutsideToReversedOverlap ||
         type == IntersectionType::kOutsideToSpikeOverlap ||
         type == IntersectionType::kInsideToSpikeOverlap;
}

bool IsOverlapType(IntersectionType type) {
  return IsOverlapToOverlapType(type) || IsOverlapToNonOverlapType(type) ||
         IsNonOverlapToOverlapType(type);
}

bool IsTraversalStartType(IntersectionType type) {
  return type == IntersectionType::kCrossInsideToOutside ||
         type == IntersectionType::kTouchInsideToInside ||
         type == IntersectionType::kAlignedOverlapToInside ||
         type == IntersectionType::kAlignedOverlapToOutside ||
         type == IntersectionType::kReversedOverlapToInside ||
         type == IntersectionType::kInsideToReversedOverlap;
}

bool IsTraversalSwitchType(IntersectionType type) {
  return type == IntersectionType::kCrossInsideToOutside ||
         type == IntersectionType::kTouchInsideToInside ||
         type == IntersectionType::kAlignedOverlapToOutside ||
         type == IntersectionType::kInsideToReversedOverlap ||
         type == IntersectionType::kAlignedOverlapToReversedOverlapCW ||
         type == IntersectionType::kReversedOverlapToAlignedOverlapCCW ||
         type == IntersectionType::kSpikeOverlapToOutside ||
         type == IntersectionType::kInsideToSpikeOverlap;
}

bool IsUnexpectedTraversalType(IntersectionType type) {
  return type == IntersectionType::kTouchInsideToOutside ||
         type == IntersectionType::kTouchOutsideToOutside ||
         type == IntersectionType::kCrossOutsideToInside ||
         type == IntersectionType::kOutsideToAlignedOverlap ||
         type == IntersectionType::kOutsideToReversedOverlap ||
         type == IntersectionType::kReversedOverlapToOutside ||
         type == IntersectionType::kReversedOverlapToReversedOverlap ||
         type == IntersectionType::kReversedOverlapToInside ||
         type == IntersectionType::kReversedOverlapToAlignedOverlapCW ||
         type == IntersectionType::kOutsideToSpikeOverlap;
}

// A vertex in the traversal, which may be the a vertex from the original
// polygon, or an intersection.
struct TraversalVertex {
  glm::vec2 position{0, 0};
  VertexType type;
  IntersectionType intx_type = IntersectionType::kInvalid;

  // In order to handle spike overlaps that begin at a point where it is
  // narrower than machine tolerance, and end at point where it is wider, we
  // need to change some vertices' intersection type to maintain a consistent
  // topology. However, we still need to know the original intersection type in
  // order to filter out collinear mid-segment intersections from overlaps.
  IntersectionType original_intx_type = IntersectionType::kInvalid;

  // This indicates whether the traversal vertex has already been used in the
  // result. Note that this isn't correlated with whether the twin has been
  // visited, as there are cases in which both the intersection and its twin
  // must be included in the result in different places.
  bool visited = false;

  // For intersection vertices, this iterator points to the matching
  // intersection in the other polygon list. For non-intersection vertices, this
  // will be default-constructed, and as such, it will not be dereferencable.
  //
  // We use std::list instead of std::vector because we need the iterators to
  // remain valid after removing elements.
  using Iterator = CyclicIterator<std::list<TraversalVertex>::iterator>;
  Iterator twin;

  TraversalVertex(glm::vec2 position_in, VertexType type_in)
      : position(position_in), type(type_in) {}

  std::string ToString() const {
    return Substitute("{$0 $1 ($2) $3}", type, intx_type, original_intx_type,
                      position);
  }
};

std::string TraversalString(const std::list<TraversalVertex> &vertices,
                            const std::list<TraversalVertex> &other_vertices) {
  std::vector<std::string> strings;
  strings.reserve(vertices.size());
  int i = 0;
  for (const auto &vertex : vertices) {
    if (vertex.type == VertexType::kNonIntersection) {
      strings.push_back(Substitute("\n  [$0] $1", i, vertex));
    } else {
      int twin_index = std::distance(other_vertices.begin(),
                                     std::list<TraversalVertex>::const_iterator(
                                         vertex.twin.BaseCurrent()));

      strings.push_back(Substitute("\n  [$0] $1 -> [$2] $3", i, vertex,
                                   twin_index, *vertex.twin));
    }
    ++i;
  }
  return absl::StrJoin(strings, "");
}

// This helper struct is used to track matching intersections when constructing
// the traversal.
struct IndexedIntersection {
  // The segment indices on the input polygons.
  std::array<int, 2> segment_idx;

  // The length ratio parameters on the intersecting segments.
  std::array<float, 2> segment_params;

  // The intersection location.
  glm::vec2 position{0, 0};

  // The positions of the intersection in the respective traversal polygons.
  std::array<std::list<TraversalVertex>::iterator, 2> traversal_it;

  IndexedIntersection(std::array<int, 2> segment_idx_in,
                      std::array<float, 2> segment_params_in,
                      glm::vec2 position_in)
      : segment_idx(segment_idx_in),
        segment_params(segment_params_in),
        position(position_in) {}

  // Returns true if lhs is strictly less than rhs with respect to sort_polygon.
  // sort_polygon and other_polygon must be the same polygons that were passed
  // to GetIntersections(). sort_index must be either 0 or 1, and indicates
  // which element of segment_idx and segment_params corresponds to
  // sort_polygon. The element not indicated by sort_index corresponds to
  // other_polygon.
  static bool LessThanWrtPolygon(int sort_index, const Polygon &sort_polygon,
                                 const Polygon &other_polygon,
                                 const IndexedIntersection &lhs,
                                 const IndexedIntersection &rhs) {
    if (lhs.segment_idx[sort_index] < rhs.segment_idx[sort_index]) return true;
    if (lhs.segment_idx[sort_index] > rhs.segment_idx[sort_index]) return false;

    if (lhs.segment_params[sort_index] < rhs.segment_params[sort_index])
      return true;
    if (lhs.segment_params[sort_index] > rhs.segment_params[sort_index])
      return false;

    int other_index = sort_index == 0 ? 1 : 0;
    if (lhs.segment_idx[other_index] == rhs.segment_idx[other_index] &&
        lhs.segment_params[other_index] == rhs.segment_params[other_index])
      // The two sides are equal.
      return false;

    // Both sides describe the same position w.r.t. to the sort index, which
    // either means it's an intersection at the vertex, or an intersection with
    // a spike.
    //
    // In the case of an intersection with a spike, we can infer from the fact
    // that the input polygons are free from self-intersections that the
    // positions only appear equal due to floating-point error. As such, there
    // is a deterministic ordering, which we can find by examining the segments.

    bool lhs_before_rhs = false;
    if ((lhs.segment_idx[other_index] + 1) % other_polygon.Size() ==
        rhs.segment_idx[other_index]) {
      // The left-hand-side segment is directly before the right-hand-side
      // segment w.r.t. other_polygon.
      lhs_before_rhs = true;
    } else if ((rhs.segment_idx[other_index] + 1) % other_polygon.Size() ==
               lhs.segment_idx[other_index]) {
      // The right-hand-side segment is directly before the left-hand-side
      // segment w.r.t. other_polygon.
      lhs_before_rhs = false;
    } else {
      // The left-hand-side and right-hand-side segments, w.r.t. other_polygon,
      // are either the same, or non-adjacent.
      return lhs.segment_idx[other_index] < rhs.segment_idx[other_index] ||
             (lhs.segment_idx[other_index] == rhs.segment_idx[other_index] &&
              lhs.segment_params[other_index] <
                  rhs.segment_params[other_index]);
    }

    auto lhs_vector =
        other_polygon.GetSegment(lhs.segment_idx[other_index]).DeltaVector();
    auto rhs_vector =
        other_polygon.GetSegment(rhs.segment_idx[other_index]).DeltaVector();
    auto common_vector =
        sort_polygon.GetSegment(lhs.segment_idx[sort_index]).DeltaVector();

    float det_lhs_common = Determinant(lhs_vector, common_vector);
    float det_rhs_common = Determinant(rhs_vector, common_vector);
    if (Determinant(lhs_vector, common_vector) == 0 ||
        Determinant(rhs_vector, common_vector) == 0) {
      // One of the segments is parallel to the common segment -- if the
      // intersection occurs at one of the endpoints of the common segment, we
      // compare w.r.t. the adjacent segment to maintain consistency.
      if (lhs.segment_params[sort_index] == 0) {
        common_vector = sort_polygon
                            .GetSegment((lhs.segment_idx[sort_index] +
                                         sort_polygon.Size() - 1) %
                                        sort_polygon.Size())
                            .DeltaVector();
        det_lhs_common = Determinant(lhs_vector, common_vector);
        det_rhs_common = Determinant(rhs_vector, common_vector);
      } else if (lhs.segment_params[sort_index] == 1) {
        common_vector = sort_polygon
                            .GetSegment((lhs.segment_idx[sort_index] + 1) %
                                        sort_polygon.Size())
                            .DeltaVector();
        det_lhs_common = Determinant(lhs_vector, common_vector);
        det_rhs_common = Determinant(rhs_vector, common_vector);
      }
    }

    if (det_lhs_common * det_rhs_common > 0) {
      // The left-hand-side and right-hand-side segments are on opposite sides
      // of the common segment, so this must be an intersection at the vertex.
      return lhs.segment_idx[other_index] < rhs.segment_idx[other_index] ||
             (lhs.segment_idx[other_index] == rhs.segment_idx[other_index] &&
              lhs.segment_params[other_index] <
                  rhs.segment_params[other_index]);
    }

    float det_lhs_rhs = Determinant(lhs_vector, rhs_vector);
    if (lhs_before_rhs) {
      return det_lhs_common * det_lhs_rhs > 0;
    } else {
      return det_rhs_common * det_lhs_rhs > 0;
    }
  }
};

glm::vec2 FindNonCoincidentPoint(const TraversalVertex::Iterator &begin,
                                 int increment) {
  auto it = begin;
  while (it->position == begin->position) {
    std::advance(it, increment);
    EXPECT(it != begin);
  }
  return it->position;
}

// Determines the intersection type, and sets it on both the TraversalVertex
// pointed to by the iterator, and its twin.
void PopulateIntersectionType(const TraversalVertex::Iterator &it) {
  ASSERT(it->type != VertexType::kNonIntersection);

  glm::vec2 intx = it->position;
  glm::vec2 lhs_prev = FindNonCoincidentPoint(it, -1);
  glm::vec2 lhs_next = FindNonCoincidentPoint(it, 1);
  glm::vec2 rhs_prev = FindNonCoincidentPoint(it->twin, -1);
  glm::vec2 rhs_next = FindNonCoincidentPoint(it->twin, 1);

  auto find_non_coincident_vertex = [](const TraversalVertex::Iterator &begin,
                                       int increment) {
    auto it = begin;
    while (it->position == begin->position ||
           it->type == VertexType::kIntersection) {
      std::advance(it, increment);
      EXPECT(it != begin);
    }
    return it->position;
  };

  // Because intersections are inserted in both traversals, overlapping segments
  // result in adjacent coincident points. Similarly, spikes result in the
  // previous and next points being coincident.
  if (lhs_prev == lhs_next && lhs_prev == rhs_prev) {
    lhs_prev = find_non_coincident_vertex(it, -1);
    lhs_next = find_non_coincident_vertex(it, 1);
    if (Orientation(lhs_prev, intx, lhs_next) == RelativePos::kLeft) {
      it->intx_type = IntersectionType::kAlignedOverlapToReversedOverlapCCW;
    } else {
      it->intx_type = IntersectionType::kAlignedOverlapToReversedOverlapCW;
    }
  } else if (lhs_prev == lhs_next && lhs_prev == rhs_next) {
    lhs_prev = find_non_coincident_vertex(it, -1);
    lhs_next = find_non_coincident_vertex(it, 1);
    if (Orientation(lhs_prev, intx, lhs_next) == RelativePos::kLeft) {
      it->intx_type = IntersectionType::kReversedOverlapToAlignedOverlapCCW;
    } else {
      it->intx_type = IntersectionType::kReversedOverlapToAlignedOverlapCW;
    }
  } else if (rhs_prev == rhs_next && rhs_prev == lhs_prev) {
    rhs_prev = find_non_coincident_vertex(it->twin, -1);
    rhs_next = find_non_coincident_vertex(it->twin, 1);
    if (Orientation(rhs_prev, intx, rhs_next) == RelativePos::kLeft) {
      it->intx_type = IntersectionType::kSpikeOverlapToOutside;
    } else {
      it->intx_type = IntersectionType::kSpikeOverlapToInside;
    }
  } else if (rhs_prev == rhs_next && rhs_prev == lhs_next) {
    rhs_prev = find_non_coincident_vertex(it->twin, -1);
    rhs_next = find_non_coincident_vertex(it->twin, 1);
    if (Orientation(rhs_prev, intx, rhs_next) == RelativePos::kLeft) {
      it->intx_type = IntersectionType::kOutsideToSpikeOverlap;
    } else {
      it->intx_type = IntersectionType::kInsideToSpikeOverlap;
    }
  } else if (lhs_prev == rhs_prev && lhs_next == rhs_next) {
    it->intx_type = IntersectionType::kAlignedOverlapToAlignedOverlap;
  } else if (lhs_prev == rhs_next && lhs_next == rhs_prev) {
    it->intx_type = IntersectionType::kReversedOverlapToReversedOverlap;
  } else if (lhs_prev == rhs_prev) {
    if (OrientationAboutTurn(lhs_prev, intx, lhs_next, rhs_next) ==
        RelativePos::kLeft) {
      it->intx_type = IntersectionType::kAlignedOverlapToOutside;
    } else {
      it->intx_type = IntersectionType::kAlignedOverlapToInside;
    }
  } else if (lhs_prev == rhs_next) {
    if (OrientationAboutTurn(lhs_prev, intx, lhs_next, rhs_prev) ==
        RelativePos::kLeft) {
      it->intx_type = IntersectionType::kReversedOverlapToInside;
    } else {
      it->intx_type = IntersectionType::kReversedOverlapToOutside;
    }
  } else if (lhs_next == rhs_prev) {
    if (OrientationAboutTurn(lhs_prev, intx, lhs_next, rhs_next) ==
        RelativePos::kLeft) {
      it->intx_type = IntersectionType::kInsideToReversedOverlap;
    } else {
      it->intx_type = IntersectionType::kOutsideToReversedOverlap;
    }
  } else if (lhs_next == rhs_next) {
    if (OrientationAboutTurn(lhs_prev, intx, lhs_next, rhs_prev) ==
        RelativePos::kLeft) {
      it->intx_type = IntersectionType::kOutsideToAlignedOverlap;
    } else {
      it->intx_type = IntersectionType::kInsideToAlignedOverlap;
    }
  } else {
    // Since this isn't an overlap, we're only concerned with the relative
    // orientation of the intersecting segments. Moving the previous and next
    // points to vertices of the input polygons doesn't change the orientation,
    // but helps with precision issues that arise when intersecting a spike.
    lhs_prev = find_non_coincident_vertex(it, -1);
    lhs_next = find_non_coincident_vertex(it, 1);
    rhs_prev = find_non_coincident_vertex(it->twin, -1);
    rhs_next = find_non_coincident_vertex(it->twin, 1);

    bool rhs_prev_inside = false;
    bool rhs_next_inside = false;
    if (it->type == VertexType::kIntersectionAtVertex) {
      // The intersection is at a vertex on the original polygon, so we need to
      // take the turn into account.
      rhs_prev_inside = OrientationAboutTurn(lhs_prev, intx, lhs_next,
                                             rhs_prev) == RelativePos::kLeft;
      rhs_next_inside = OrientationAboutTurn(lhs_prev, intx, lhs_next,
                                             rhs_next) == RelativePos::kLeft;
    } else {
      // The intersection is in the middle of a segment, so we only need the
      // orientation w.r.t. that segment. In addition, by not using the
      // intersection point, we aren't affected by any error in the intersection
      // result.
      rhs_prev_inside =
          Orientation(lhs_prev, lhs_next, rhs_prev) == RelativePos::kLeft;
      rhs_next_inside =
          Orientation(lhs_prev, lhs_next, rhs_next) == RelativePos::kLeft;
    }

    if (rhs_prev_inside == rhs_next_inside) {
      bool rhs_is_ccw = OrientationAboutTurn(lhs_prev, intx, rhs_next,
                                             rhs_prev) == RelativePos::kLeft;
      if (rhs_next_inside) {
        if (rhs_is_ccw) {
          it->intx_type = IntersectionType::kTouchInsideToOutside;
        } else {
          it->intx_type = IntersectionType::kTouchInsideToInside;
        }
      } else {
        if (rhs_is_ccw) {
          it->intx_type = IntersectionType::kTouchOutsideToOutside;
        } else {
          it->intx_type = IntersectionType::kTouchOutsideToInside;
        }
      }
    } else {
      if (rhs_next_inside) {
        it->intx_type = IntersectionType::kCrossInsideToOutside;
      } else {
        it->intx_type = IntersectionType::kCrossOutsideToInside;
      }
    }
  }

  it->original_intx_type = it->intx_type;
  it->twin->intx_type = TwinIntersectionType(it->intx_type);
  it->twin->original_intx_type = it->twin->intx_type;
}

// Constructs one of the traversal polygons, by sorting the intersections w.r.t.
// that polygon, and merging the lists, populating the traversal indices in the
// IndexedIntersections.
// Parameter sort_idx must be 0 or 1, and indicates whether to use the first or
// second element of the index and parameter pairs.
std::list<TraversalVertex> MergeVerticesAndIntersections(
    int sort_idx, const Polygon &sort_polygon, const Polygon &other_polygon,
    std::vector<IndexedIntersection> *intersections) {
  std::sort(
      intersections->begin(), intersections->end(),
      [sort_idx, &sort_polygon, &other_polygon](
          const IndexedIntersection &lhs, const IndexedIntersection &rhs) {
        return IndexedIntersection::LessThanWrtPolygon(sort_idx, sort_polygon,
                                                       other_polygon, lhs, rhs);
      });

  // Given a segment/parameter pair and the number of points in the polygon,
  // returns the index of the polygon vertex coincident to the given
  // segment/parameter pair, or -1 if there is none.
  auto coincident_vertex = [](int intx_segment, float intx_param,
                              int polygon_size) {
    if (intx_param == 0) {
      return intx_segment;
    } else if (intx_param == 1) {
      return (intx_segment + 1) % polygon_size;
    } else {
      return -1;
    }
  };

  // Note that we can't use std::merge() to perform the merge, because it
  // wouldn't allow us to maintain the intersection traversal indices.
  std::list<TraversalVertex> vertices;
  int polygon_idx = 0;
  int intx_idx = 0;
  while (polygon_idx < sort_polygon.Size() ||
         intx_idx < intersections->size()) {
    int intx_segment = std::numeric_limits<int>::infinity();
    float intx_param = std::numeric_limits<float>::infinity();
    if (intx_idx < intersections->size()) {
      intx_segment = (*intersections)[intx_idx].segment_idx[sort_idx];
      intx_param = (*intersections)[intx_idx].segment_params[sort_idx];
    }

    if (intx_idx >= intersections->size() || polygon_idx < intx_segment ||
        (polygon_idx == intx_segment && intx_param > 0)) {
      bool is_previous_intx_at_vertex = false;
      if (!intersections->empty()) {
        const auto &previous_intx = intx_idx == 0
                                        ? intersections->back()
                                        : (*intersections)[intx_idx - 1];
        is_previous_intx_at_vertex =
            polygon_idx ==
            coincident_vertex(previous_intx.segment_idx[sort_idx],
                              previous_intx.segment_params[sort_idx],
                              sort_polygon.Size());
      }
      if (!is_previous_intx_at_vertex) {
        vertices.emplace_back(sort_polygon[polygon_idx],
                              VertexType::kNonIntersection);
      }
      ++polygon_idx;
    } else {
      bool is_at_vertex =
          coincident_vertex(intx_segment, intx_param, sort_polygon.Size()) ==
          polygon_idx % sort_polygon.Size();
      vertices.emplace_back((*intersections)[intx_idx].position,
                            is_at_vertex ? VertexType::kIntersectionAtVertex
                                         : VertexType::kIntersection);
      (*intersections)[intx_idx].traversal_it[sort_idx] = --vertices.end();
      ++intx_idx;
    }
  }
  return vertices;
}

void SnapIntersectionsToVertices(
    int sort_idx, const Polygon &polygon,
    std::vector<IndexedIntersection> *intersections) {
  for (auto &intx : *intersections) {
    auto segment = polygon.GetSegment(intx.segment_idx[sort_idx]);
    if (intx.segment_params[sort_idx] >= 1 - kSnappingTol ||
        RelativeErrorWithinSnappingTol(intx.position, segment.to)) {
      intx.segment_params[sort_idx] = 1;
      intx.position = segment.to;
    } else if (intx.segment_params[sort_idx] <= kSnappingTol ||
               RelativeErrorWithinSnappingTol(intx.position, segment.from)) {
      intx.segment_params[sort_idx] = 0;
      intx.position = segment.from;
    }
  }
}

void SnapMidSegmentIntersections(
    int sort_index, std::vector<IndexedIntersection> *intersections) {
  // We don't need the nuance of IndexedIntersection::LessThanWrtPolygon() here,
  // so we use a simpler sorting method.
  auto simple_less_than = [&sort_index](const IndexedIntersection &lhs,
                                        const IndexedIntersection &rhs) {
    return (lhs.segment_idx[sort_index] < rhs.segment_idx[sort_index]) ||
           (lhs.segment_idx[sort_index] == rhs.segment_idx[sort_index] &&
            lhs.segment_params[sort_index] < rhs.segment_params[sort_index]);
  };
  std::sort(intersections->begin(), intersections->end(), simple_less_than);

  for (int i = 1; i < intersections->size(); ++i) {
    const auto &previous_intx = (*intersections)[i - 1];
    auto &current_intx = (*intersections)[i];
    if (previous_intx.segment_idx[sort_index] ==
            current_intx.segment_idx[sort_index] &&
        previous_intx.segment_params[sort_index] + kSnappingTol >
            current_intx.segment_params[sort_index] &&
        RelativeErrorWithinSnappingTol(previous_intx.position,
                                       current_intx.position)) {
      current_intx.position = previous_intx.position;
      current_intx.segment_params[sort_index] =
          previous_intx.segment_params[sort_index];
    }
  }
}

// Gets the intersections of the two polygons and pre-processes them so that
// they may be merged into the traversal. Note that the IndexedIntersections'
// traversal iterators will not yet be populated.
std::vector<IndexedIntersection> GetIntersections(const Polygon &lhs_polygon,
                                                  const Polygon &rhs_polygon) {
  std::vector<PolygonIntersection> raw_intersections;
  Intersection(lhs_polygon, rhs_polygon, &raw_intersections);

  SLOG(SLOG_BOOLEAN_OPERATION, "Raw Intx: $0", raw_intersections);

  std::vector<IndexedIntersection> intersections;
  intersections.reserve(raw_intersections.size());
  for (const auto &intx : raw_intersections) {
    intersections.emplace_back(
        intx.indices,
        std::array<float, 2>{{intx.intersection.segment1_interval[0],
                              intx.intersection.segment2_interval[0]}},
        intx.intersection.intx.from);
    if (intx.intersection.segment1_interval[0] !=
        intx.intersection.segment1_interval[1]) {
      intersections.emplace_back(
          intx.indices,
          std::array<float, 2>{{intx.intersection.segment1_interval[1],
                                intx.intersection.segment2_interval[1]}},
          intx.intersection.intx.to);
    }
  }

  SnapMidSegmentIntersections(0, &intersections);
  SnapMidSegmentIntersections(1, &intersections);

  SnapIntersectionsToVertices(0, lhs_polygon, &intersections);
  SnapIntersectionsToVertices(1, rhs_polygon, &intersections);

  return intersections;
}

// Removes the vertex pointed to by the given iterator, and its twin, from the
// traversals, and returns an iterator pointing to the vertex after the one that
// was removed.
TraversalVertex::Iterator RemoveFromTraversals(
    TraversalVertex::Iterator it, std::list<TraversalVertex> *vertices,
    std::list<TraversalVertex> *other_vertices) {
  ASSERT(it->type != VertexType::kNonIntersection);
  ASSERT(vertices->size() > 1);
  ASSERT(other_vertices->size() > 1);

  // We need to take care not to remove the first element, as that would
  // invalidate all of the cyclic iterators. Instead, we copy the value of the
  // second element into the first, remove the second element, and re-link the
  // twinned intersections.

  auto remove_from_one_side = [](TraversalVertex::Iterator it,
                                 std::list<TraversalVertex> *vertices) {
    if (it.BaseCurrent() == vertices->begin()) {
      *it = *std::next(it);
      if (it->type != VertexType::kNonIntersection) it->twin->twin = it;
      vertices->erase(std::next(it).BaseCurrent());
      return it;
    } else {
      auto after_it = vertices->erase(it.BaseCurrent());
      if (after_it == vertices->end()) {
        return MakeCyclicIterator(vertices->begin(), vertices->end());
      } else {
        return MakeCyclicIterator(vertices->begin(), vertices->end(), after_it);
      }
    }
  };

  remove_from_one_side(it->twin, other_vertices);
  return remove_from_one_side(it, vertices);
}

void CorrectTopologyForSpikes(std::list<TraversalVertex> *vertices,
                              std::list<TraversalVertex> *other_vertices) {
  for (auto base_it = vertices->begin(); base_it != vertices->end();
       ++base_it) {
    auto it0 = MakeCyclicIterator(vertices->begin(), vertices->end(), base_it);
    auto it1 = std::next(it0);
    auto it2 = std::next(it1);
    auto it3 = std::next(it2);

    if (it0->type == VertexType::kNonIntersection || it0->type != it1->type ||
        it0->type != it2->type || it0->type != it3->type ||
        it0->position != it1->position || it0->position != it2->position ||
        it0->position != it3->position)
      continue;

    if ((it0->intx_type == it2->intx_type && it1->intx_type == it3->intx_type &&
         (it0->twin == std::prev(it2->twin) ||
          it0->twin == std::next(it2->twin)) &&
         (it1->twin == std::prev(it3->twin) ||
          it1->twin == std::next(it3->twin))) ||
        (it0->intx_type == it3->intx_type && it1->intx_type == it2->intx_type &&
         (it0->twin == std::prev(it3->twin) ||
          it0->twin == std::next(it3->twin)) &&
         (it1->twin == std::prev(it2->twin) ||
          it1->twin == std::next(it2->twin)))) {
      // We have to erase the vertices individually, because, while they are
      // adjacent w.r.t. the cyclic traversal, they might not be adjacent in the
      // underlying list.
      // Note that, because we avoid erasing the first element, it2 might be
      // invalidated by removing it1 -- use the return value of
      // RemoveFromTraversals instead.
      auto after_it1 = RemoveFromTraversals(it1, vertices, other_vertices);
      RemoveFromTraversals(after_it1, vertices, other_vertices);
    }
  }
}

void RemoveDuplicateIntersections(std::list<TraversalVertex> *lhs_vertices,
                                  std::list<TraversalVertex> *rhs_vertices) {
  auto base_it = lhs_vertices->begin();
  while (base_it != lhs_vertices->end()) {
    auto it =
        MakeCyclicIterator(lhs_vertices->begin(), lhs_vertices->end(), base_it);
    if (it->type != VertexType::kNonIntersection &&
        it->type == std::prev(it)->type &&
        it->intx_type == std::prev(it)->intx_type &&
        it->position == std::prev(it)->position &&
        (std::prev(it)->twin == std::prev(it->twin) ||
         std::prev(it)->twin == std::next(it->twin))) {
      base_it =
          RemoveFromTraversals(it, lhs_vertices, rhs_vertices).BaseCurrent();
    } else {
      ++base_it;
    }
  }
}

void CorrectIntersectionTypesForSpikes(std::list<TraversalVertex> *vertices) {
  auto it = MakeCyclicIterator(vertices->begin(), vertices->end());
  do {
    if (it->type != VertexType::kIntersectionAtVertex) {
      ++it;
      continue;
    }

    auto previous = std::prev(it);
    auto next = std::next(it);
    if (next->type == VertexType::kIntersectionAtVertex) {
      if ((it->intx_type == IntersectionType::kOutsideToAlignedOverlap &&
           next->intx_type == IntersectionType::kInsideToReversedOverlap) ||
          (it->intx_type == IntersectionType::kOutsideToReversedOverlap &&
           next->intx_type == IntersectionType::kInsideToAlignedOverlap) ||
          (it->intx_type == IntersectionType::kOutsideToReversedOverlap &&
           next->intx_type == IntersectionType::kTouchOutsideToInside) ||
          (it->intx_type == IntersectionType::kOutsideToAlignedOverlap &&
           next->intx_type == IntersectionType::kTouchInsideToInside)) {
        it->intx_type = IntersectionType::kCrossOutsideToInside;
      } else if ((it->intx_type == IntersectionType::kInsideToAlignedOverlap &&
                  next->intx_type ==
                      IntersectionType::kOutsideToReversedOverlap) ||
                 (it->intx_type == IntersectionType::kInsideToReversedOverlap &&
                  next->intx_type ==
                      IntersectionType::kOutsideToAlignedOverlap) ||
                 (it->intx_type == IntersectionType::kInsideToAlignedOverlap &&
                  next->intx_type ==
                      IntersectionType::kTouchOutsideToOutside) ||
                 (it->intx_type == IntersectionType::kInsideToReversedOverlap &&
                  next->intx_type == IntersectionType::kTouchInsideToOutside)) {
        it->intx_type = IntersectionType::kCrossInsideToOutside;
      }
    } else if (previous->type == VertexType::kIntersectionAtVertex) {
      if ((previous->intx_type == IntersectionType::kAlignedOverlapToOutside &&
           it->intx_type == IntersectionType::kReversedOverlapToInside) ||
          (previous->intx_type == IntersectionType::kReversedOverlapToOutside &&
           it->intx_type == IntersectionType::kAlignedOverlapToInside) ||
          (previous->intx_type == IntersectionType::kTouchInsideToOutside &&
           it->intx_type == IntersectionType::kReversedOverlapToInside) ||
          (previous->intx_type == IntersectionType::kTouchOutsideToOutside &&
           it->intx_type == IntersectionType::kAlignedOverlapToInside)) {
        it->intx_type = IntersectionType::kCrossOutsideToInside;
      } else if ((previous->intx_type ==
                      IntersectionType::kAlignedOverlapToInside &&
                  it->intx_type ==
                      IntersectionType::kReversedOverlapToOutside) ||
                 (previous->intx_type ==
                      IntersectionType::kReversedOverlapToInside &&
                  it->intx_type ==
                      IntersectionType::kAlignedOverlapToOutside) ||
                 (previous->intx_type ==
                      IntersectionType::kTouchInsideToInside &&
                  it->intx_type ==
                      IntersectionType::kAlignedOverlapToOutside) ||
                 (previous->intx_type ==
                      IntersectionType::kTouchOutsideToInside &&
                  it->intx_type ==
                      IntersectionType::kReversedOverlapToOutside)) {
        it->intx_type = IntersectionType::kCrossInsideToOutside;
      }
    }

    it->twin->intx_type = TwinIntersectionType(it->intx_type);
    ++it;
  } while (it.BaseCurrent() != vertices->begin());
}

// Constructs the traversal polygons, with the intersections merged in order
// and linked to their twins. Returns true if intersections were found.
bool ConstructTraversalPolygons(const Polygon &lhs_polygon,
                                const Polygon &rhs_polygon,
                                std::list<TraversalVertex> *lhs_vertices,
                                std::list<TraversalVertex> *rhs_vertices) {
  auto intersections = GetIntersections(lhs_polygon, rhs_polygon);

  *lhs_vertices = MergeVerticesAndIntersections(0, lhs_polygon, rhs_polygon,
                                                &intersections);
  *rhs_vertices = MergeVerticesAndIntersections(1, rhs_polygon, lhs_polygon,
                                                &intersections);
  if (intersections.empty()) return false;

  // Link the intersections.
  for (const auto &intx : intersections) {
    intx.traversal_it[0]->twin = MakeCyclicIterator(
        rhs_vertices->begin(), rhs_vertices->end(), intx.traversal_it[1]);
    intx.traversal_it[1]->twin = MakeCyclicIterator(
        lhs_vertices->begin(), lhs_vertices->end(), intx.traversal_it[0]);
  }

  // Classify the intersections.
  auto it = MakeCyclicIterator(lhs_vertices->begin(), lhs_vertices->end());
  do {
    if (it->type != VertexType::kNonIntersection) PopulateIntersectionType(it);
    ++it;
  } while (it.BaseCurrent() != lhs_vertices->begin());

  CorrectTopologyForSpikes(lhs_vertices, rhs_vertices);
  CorrectTopologyForSpikes(rhs_vertices, lhs_vertices);

  RemoveDuplicateIntersections(lhs_vertices, rhs_vertices);
  RemoveDuplicateIntersections(rhs_vertices, lhs_vertices);

  CorrectIntersectionTypesForSpikes(lhs_vertices);
  CorrectIntersectionTypesForSpikes(rhs_vertices);

  return true;
}

// Iterates forward from *pointer_to_iter until it finds a appropriate vertex to
// begin traversing the intersection polygon.
bool FindNextTraversalStart(const TraversalVertex::Iterator &begin,
                            TraversalVertex::Iterator *pointer_to_iter) {
  do {
    if (!(*pointer_to_iter)->visited &&
        (*pointer_to_iter)->type != VertexType::kNonIntersection &&
        IsTraversalStartType((*pointer_to_iter)->intx_type))
      return true;
    ++(*pointer_to_iter);
  } while (*pointer_to_iter != begin);
  return false;
}

// Traverses the polygons from the start vertex, tracing the intersection
// polygon. max_traversal_size is used as a safety mechanism to prevent an
// infinite loop in the case of an error, and should be the sum of the sizes of
// the left- and right-hand vertex lists.
// This returns absl::nullopt if an error occurs.
absl::optional<Polygon> TraverseLinkedVertexLists(
    TraversalVertex::Iterator begin, int max_traversal_size) {
  SLOG(SLOG_BOOLEAN_OPERATION, "Starting Traversal");
  std::vector<glm::vec2> traversal;
  auto append_to_traversal = [&traversal](glm::vec2 v) {
    if (traversal.empty() || traversal.back() != v) traversal.emplace_back(v);
  };
  auto it = begin;
  do {
    SLOG(SLOG_BOOLEAN_OPERATION, "Traversing: $0", *it);

    if (it != begin && IsUnexpectedTraversalType(it->intx_type)) {
      SLOG(SLOG_BOOLEAN_OPERATION,
           "Encountered unexpected intersection type in traversal");
      return absl::nullopt;
    } else if (traversal.size() > max_traversal_size) {
      SLOG(SLOG_BOOLEAN_OPERATION,
           "Traversal has exceeded maximum possible size for the given input "
           "polygons.");
      return absl::nullopt;
    }

    it->visited = true;
    if (IsOverlapType(it->intx_type)) it->twin->visited = true;
    if (it->type == VertexType::kNonIntersection) {
      // This is a vertex from one of the input polygons.
      append_to_traversal(it->position);
    } else if (IsTraversalSwitchType(it->intx_type)) {
      // We've come to an intersection where we leave the current polygon, so we
      // must switch to the other traversal.

      // If this intersection is at the beginning or end of an overlap in the
      // middle of a segment, we don't want to add the extra collinear point.
      bool mid_segment_overlap =
          (it->twin->type == VertexType::kIntersection &&
           (it->intx_type == IntersectionType::kAlignedOverlapToOutside ||
            it->intx_type ==
                IntersectionType::kAlignedOverlapToReversedOverlapCW)) ||
          (it == begin && it->type == VertexType::kIntersection &&
           it->original_intx_type == IntersectionType::kInsideToAlignedOverlap);
      if (!mid_segment_overlap) {
        append_to_traversal(it->position);
      }
      it = it->twin;
      SLOG(SLOG_BOOLEAN_OPERATION, "Switched traversal: $0", *it);
    } else if (it->type == VertexType::kIntersectionAtVertex ||
               (it == begin &&
                it->intx_type == IntersectionType::kReversedOverlapToInside)) {
      // We're not switching traversals at this vertex, but it was in one of the
      // input polygons, so we still need to keep it.
      append_to_traversal(it->position);
    }

    ++it;
  } while (it != begin && !(IsOverlapType(it->intx_type) && it->twin == begin));

  if (traversal.size() > 1 && traversal.front() == traversal.back())
    traversal.pop_back();

  // If the traversal is a degenerate polygon, throw it away.
  if (traversal.size() < 3) {
    SLOG(SLOG_BOOLEAN_OPERATION, "Discarding degenerate traversal: $0",
         traversal);
    return absl::nullopt;
  }

  SLOG(SLOG_BOOLEAN_OPERATION, "Completed Traversal: $0", traversal);
  return Polygon(traversal);
}

// Finds a point on the traversal that doesn't lie on the other polygon -- this
// point can be used for checking containment. If no valid point is found, it
// returns absl::nullopt, indicating that each segment on the traversal lies on
// the other polygon.
absl::optional<glm::vec2> FindPointForContainmentCheck(
    TraversalVertex::Iterator begin) {
  auto it = begin;
  do {
    if (it->type == VertexType::kNonIntersection) {
      return it->position;
    } else if (IsOverlapToNonOverlapType(it->intx_type) &&
               it->position != std::next(it)->position) {
      return .5f * (it->position + std::next(it)->position);
    } else if (IsNonOverlapToOverlapType(it->intx_type) &&
               it->position != std::prev(it)->position) {
      return .5f * (it->position + std::prev(it)->position);
    }
    ++it;
  } while (it != begin);
  return absl::nullopt;
}

enum class IntersectionResult {
  kError,
  kIntersection,
  kLeftInsideRight,
  kRightInsideLeft,
  kDisjoint,
  kCompleteOverlap
};

IntersectionResult IntersectionHelper(Polygon lhs_polygon, Polygon rhs_polygon,
                                      std::vector<Polygon> *result) {
  lhs_polygon.RemoveDuplicatePoints();
  rhs_polygon.RemoveDuplicatePoints();
  SLOG(SLOG_BOOLEAN_OPERATION, "LHS Polygon: $0", lhs_polygon);
  SLOG(SLOG_BOOLEAN_OPERATION, "RHS Polygon: $0", rhs_polygon);
  if (lhs_polygon.Size() < 3 || rhs_polygon.Size() < 3)
    return IntersectionResult::kError;

  // We find the difference by finding the intersection of the base polygon and
  // the complement of the cutting polygon.
  std::list<TraversalVertex> lhs_vertices;
  std::list<TraversalVertex> rhs_vertices;
  bool found_intersections = ConstructTraversalPolygons(
      lhs_polygon, rhs_polygon, &lhs_vertices, &rhs_vertices);

  SLOG(SLOG_BOOLEAN_OPERATION, "LHS Traversal: $0",
       TraversalString(lhs_vertices, rhs_vertices));
  SLOG(SLOG_BOOLEAN_OPERATION, "RHS Traversal: $0",
       TraversalString(rhs_vertices, lhs_vertices));

  int max_traversal_size = lhs_vertices.size() + rhs_vertices.size();
  bool found_degenerate_traversal = false;
  if (found_intersections) {
    auto begin = MakeCyclicIterator(lhs_vertices.begin(), lhs_vertices.end());
    auto it = begin;
    while (FindNextTraversalStart(begin, &it)) {
      absl::optional<Polygon> traversal =
          TraverseLinkedVertexLists(it, max_traversal_size);
      if (traversal) {
        result->emplace_back(std::move(traversal.value()));
      } else {
        found_degenerate_traversal = true;
      }
      ++it;
    }
  }

  if (!result->empty() || found_degenerate_traversal) {
    SLOG(SLOG_BOOLEAN_OPERATION, "Result: $0", *result);
    return IntersectionResult::kIntersection;
  }

  SLOG(SLOG_BOOLEAN_OPERATION,
       "Traversal yielded empty result, checking containment.");

  absl::optional<glm::vec2> lhs_test_point = FindPointForContainmentCheck(
      MakeCyclicIterator(lhs_vertices.begin(), lhs_vertices.end()));
  absl::optional<glm::vec2> rhs_test_point = FindPointForContainmentCheck(
      MakeCyclicIterator(rhs_vertices.begin(), rhs_vertices.end()));
  SLOG(SLOG_BOOLEAN_OPERATION, "LHS test point: $0",
       lhs_test_point.has_value() ? Str(lhs_test_point.value()) : "NULL");
  SLOG(SLOG_BOOLEAN_OPERATION, "RHS test point: $0",
       rhs_test_point.has_value() ? Str(rhs_test_point.value()) : "NULL");

  if (!lhs_test_point.has_value() || !rhs_test_point.has_value()) {
    SLOG(SLOG_BOOLEAN_OPERATION,
         "Could not find containment point(s), polygons must completely "
         "overlap");
    return IntersectionResult::kCompleteOverlap;
  } else if (lhs_polygon.WindingNumber(*rhs_test_point) != 0) {
    SLOG(SLOG_BOOLEAN_OPERATION, "RHS polygon inside LHS polygon");
    ASSERT(rhs_polygon.WindingNumber(*lhs_test_point) == 0);
    return IntersectionResult::kRightInsideLeft;
  } else if (rhs_polygon.WindingNumber(*lhs_test_point) != 0) {
    SLOG(SLOG_BOOLEAN_OPERATION, "LHS polygon inside RHS polygon");
    ASSERT(lhs_polygon.WindingNumber(*rhs_test_point) == 0);
    return IntersectionResult::kLeftInsideRight;
  } else {
    SLOG(SLOG_BOOLEAN_OPERATION, "Polygons are disjoint");
    return IntersectionResult::kDisjoint;
  }
}

}  // namespace

std::vector<Polygon> Difference(const Polygon &base_polygon,
                                const Polygon &cutting_polygon) {
  ASSERT(base_polygon.SignedArea() >= 0);
  ASSERT(cutting_polygon.SignedArea() >= 0);

  // Reversing the cutting polygon is equivalent to taking its complement.
  Polygon reversed_cutting_polygon = cutting_polygon.Reversed();

  // We find the difference by finding the intersection of the base polygon and
  // the complement of the cutting polygon.
  std::vector<Polygon> result;
  IntersectionResult result_type =
      IntersectionHelper(base_polygon, reversed_cutting_polygon, &result);
  switch (result_type) {
    case IntersectionResult::kError:
      return {};
    case IntersectionResult::kIntersection:
      return result;
    case IntersectionResult::kLeftInsideRight:
      return {};
    case IntersectionResult::kRightInsideLeft:
      // The cutting polygon lies inside the base polygon -- reverse the
      // cutting polygon to indicate that it is a hole.
      return {base_polygon, reversed_cutting_polygon};
    case IntersectionResult::kDisjoint:
      return {base_polygon};
    case IntersectionResult::kCompleteOverlap:
      return {};
  }
}

std::vector<Polygon> Intersection(const Polygon &lhs_polygon,
                                  const Polygon &rhs_polygon) {
  ASSERT(lhs_polygon.SignedArea() >= 0);
  ASSERT(rhs_polygon.SignedArea() >= 0);

  std::vector<Polygon> result;
  IntersectionResult result_type =
      IntersectionHelper(lhs_polygon, rhs_polygon, &result);
  switch (result_type) {
    case IntersectionResult::kError:
      return {};
    case IntersectionResult::kIntersection:
      return result;
    case IntersectionResult::kLeftInsideRight:
      return {lhs_polygon};
    case IntersectionResult::kRightInsideLeft:
      return {rhs_polygon};
    case IntersectionResult::kDisjoint:
      return {};
    case IntersectionResult::kCompleteOverlap:
      return {lhs_polygon};
  }
}

}  // namespace geometry

template <>
std::string Str(const geometry::VertexType &type) {
  using geometry::VertexType;
  switch (type) {
    case VertexType::kNonIntersection:
      return "V ";
    case VertexType::kIntersection:
      return " X";
    case VertexType::kIntersectionAtVertex:
      return "VX";
  }
}

template <>
std::string Str(const geometry::IntersectionType &type) {
  using geometry::IntersectionType;
  switch (type) {
    case IntersectionType::kInvalid:
      return "N/A";
    case IntersectionType::kCrossInsideToOutside:
      return "XIO";
    case IntersectionType::kCrossOutsideToInside:
      return "XOI";
    case IntersectionType::kTouchInsideToInside:
      return "TII";
    case IntersectionType::kTouchInsideToOutside:
      return "TIO";
    case IntersectionType::kTouchOutsideToInside:
      return "TOI";
    case IntersectionType::kTouchOutsideToOutside:
      return "TOO";
    case IntersectionType::kOutsideToAlignedOverlap:
      return "O2A";
    case IntersectionType::kInsideToAlignedOverlap:
      return "I2A";
    case IntersectionType::kAlignedOverlapToOutside:
      return "A2O";
    case IntersectionType::kAlignedOverlapToInside:
      return "A2I";
    case IntersectionType::kAlignedOverlapToAlignedOverlap:
      return "A2A";
    case IntersectionType::kOutsideToReversedOverlap:
      return "O2R";
    case IntersectionType::kInsideToReversedOverlap:
      return "I2R";
    case IntersectionType::kReversedOverlapToOutside:
      return "R2O";
    case IntersectionType::kReversedOverlapToInside:
      return "R2I";
    case IntersectionType::kReversedOverlapToReversedOverlap:
      return "R2R";
    case IntersectionType::kAlignedOverlapToReversedOverlapCCW:
      return "AR+";
    case IntersectionType::kSpikeOverlapToOutside:
      return "S2O";
    case IntersectionType::kAlignedOverlapToReversedOverlapCW:
      return "AR-";
    case IntersectionType::kSpikeOverlapToInside:
      return "S2I";
    case IntersectionType::kReversedOverlapToAlignedOverlapCCW:
      return "RA+";
    case IntersectionType::kOutsideToSpikeOverlap:
      return "O2S";
    case IntersectionType::kReversedOverlapToAlignedOverlapCW:
      return "RA-";
    case IntersectionType::kInsideToSpikeOverlap:
      return "I2S";
  }
}

}  // namespace ink
