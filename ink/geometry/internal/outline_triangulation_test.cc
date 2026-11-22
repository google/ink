// Copyright 2026 Google LLC
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

#include "ink/geometry/internal/outline_triangulation.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ink/geometry/internal/outline_processing.h"
#include "ink/geometry/point.h"
#include "ink/geometry/triangle.h"

namespace ink::geometry_internal {
namespace {

using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

// Returns true if the point is contained in some triangle.
bool ContainsPoint(const std::vector<Point>& points,
                   const std::vector<std::array<uint32_t, 3>>& triangles,
                   Point p) {
  return std::any_of(triangles.begin(), triangles.end(),
                     [&points, p](const std::array<uint32_t, 3>& tri) {
                       return Triangle{points[tri[0]], points[tri[1]],
                                       points[tri[2]]}
                           .Contains(p);
                     });
}

// Returns true if all indices in the triangles are valid indices into the
// points array.
bool HasValidIndices(const std::vector<Point>& points,
                     const std::vector<std::array<uint32_t, 3>>& triangles) {
  return std::all_of(triangles.begin(), triangles.end(),
                     [&points](const std::array<uint32_t, 3>& tri) {
                       return tri[0] < points.size() &&
                              tri[1] < points.size() && tri[2] < points.size();
                     });
}

// Returns true if all triangles are nondegenerate and counter-clockwise.
bool AreCounterClockwise(
    const std::vector<Point>& points,
    const std::vector<std::array<uint32_t, 3>>& triangles) {
  return std::all_of(triangles.begin(), triangles.end(),
                     [&points](const std::array<uint32_t, 3>& tri) {
                       return Triangle{points[tri[0]], points[tri[1]],
                                       points[tri[2]]}
                                  .SignedArea() > 0;
                     });
}

TEST(TriangulateTest, Empty) {
  auto [points, triangles] = Triangulate(ShapeOutline());
  EXPECT_THAT(points, IsEmpty());
  EXPECT_THAT(triangles, IsEmpty());
}

TEST(TriangulateTest, BasicTriangle) {
  //         C
  //        / \
  //       /   \
  //      A-----B
  Point A{0, 0}, B{2, 0}, C{1, 1};
  ShapeOutline outline({{{A, B}, 1}, {{A, C, B}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C));
  EXPECT_THAT(triangles, SizeIs(1));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 0.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{1, 1.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{1, -0.5f}));
}

TEST(TriangulateTest, TriangleWithExtraCollinearVertex) {
  //         C
  //        / \
  //       /   \
  //      A--M--B
  Point A{0, 0}, M{1, 0}, B{2, 0}, C{1, 2};
  ShapeOutline outline({{{A, M, B}, 1}, {{A, C, B}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, M, B, C));
  EXPECT_THAT(triangles, SizeIs(2));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{0.5f, 0.5f}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1.5f, 0.5f}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1.0f, 1.0f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{0.5f, -0.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{0.5f, 1.5f}));
}

TEST(TriangulateTest, BasicQuad) {
  //    D-----------C
  //    |           |
  //    |           |
  //    |           |
  //    A-----------B
  // Both chains span the same y-range, so `ShapeOutline` orders the upper chain
  // first.
  Point A{0, 0}, B{2, 0}, C{2, 2}, D{0, 2};
  ShapeOutline outline({{{A, B, C}, 1}, {{A, D, C}, -1}});
  ASSERT_EQ(outline.Chains()[0].Orientation(), -1);

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D));
  EXPECT_THAT(triangles, SizeIs(2));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{0.5f, 1.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{3, 3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{-1, 1}));
}

TEST(TriangulateTest, ConcavePolygon) {
  //       B       D
  //      / \     / \
  //     /   \   /   \
  //    /     \ /     \
  //   /       C       \
  //  /                 \
  // A-------------------E

  Point A{0, 0}, B{2, 4}, C{4, 2}, D{6, 4}, E{8, 0};
  ShapeOutline outline({{{A, E}, 1}, {{A, B, C, D, E}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, E));
  EXPECT_THAT(triangles, SizeIs(3));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{2, 2}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{4, 1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{6, 2}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{4, 3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{1, 3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{7, 3}));
}

TEST(TriangulateTest, ConcaveLowerChain) {
  // A-------------------E
  //  \                 /
  //   \       C       /
  //    \     / \     /
  //     \   /   \   /
  //      \ /     \ /
  //       B       D
  Point A{0, 0}, B{2, -4}, C{4, -2}, D{6, -4}, E{8, 0};
  ShapeOutline outline({{{A, B, C, D, E}, 1}, {{A, E}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, E));
  EXPECT_THAT(triangles, SizeIs(3));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{2, -2}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{4, -1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{6, -2}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{4, -3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{1, -3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{7, -3}));
}

TEST(TriangulateTest, ConcaveRight) {
  //   E----------D
  //   |         /
  //   |        /
  //   |       C
  //   |        \
  //   |         \
  //   A----------B
  Point A{0, 0}, B{6, 0}, C{2, 2}, D{6, 4}, E{0, 4};
  ShapeOutline outline(
      {{{A, B}, 1}, {{C, B}, -1}, {{C, D}, 1}, {{A, E, D}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, D, E, C));
  EXPECT_THAT(triangles, SizeIs(3));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 2}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{5, 2}));
}

TEST(TriangulateTest, ConcaveLeft) {
  //  D------------C
  //   \           |
  //    \          |
  //     E         |
  //    /          |
  //   /           |
  //  A------------B
  Point A{0, 0}, B{6, 0}, C{6, 4}, D{0, 4}, E{4, 2};
  ShapeOutline outline(
      {{{A, B, C}, 1}, {{A, E}, -1}, {{D, E}, 1}, {{D, C}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, E));
  EXPECT_THAT(triangles, SizeIs(3));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{5, 2}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{1, 2}));
}

TEST(TriangulateTest, SquareWithHole) {
  //  D-----------C
  //  |     Q     |
  //  |    / \    |
  //  |   P   R   |
  //  |    \ /    |
  //  |     S     |
  //  A-----------B

  Point A{0, 0}, B{4, 0}, C{4, 4}, D{0, 4};
  Point P{1, 2}, Q{2, 3}, R{3, 2}, S{2, 1};
  ShapeOutline outline(
      {{{A, B, C}, 1}, {{A, D, C}, -1}, {{P, Q, R}, 1}, {{P, S, R}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, P, Q, R, S));
  EXPECT_THAT(triangles, SizeIs(8));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{0.5f, 2}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{3.5f, 2}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{2, 0.5f}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{2, 3.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{2, 2}));
}

TEST(TriangulateTest, HolesSideBySide) {
  //  D-------------------C
  //  |     F       J     |
  //  |    / \     / \    |
  //  |   E   G   I   K   |
  //  |    \ /     \ /    |
  //  |     H       L     |
  //  A-------------------B
  Point A{0, 0}, B{8, 0}, C{8, 4}, D{0, 4};
  Point E{1.5f, 2}, F{2.5f, 3}, G{3.5f, 2}, H{2.5f, 1};
  Point I{4.5f, 2}, J{5.5f, 3}, K{6.5f, 2}, L{5.5f, 1};
  ShapeOutline outline({{{A, B, C}, 1},
                        {{A, D, C}, -1},
                        {{E, F, G}, 1},
                        {{E, H, G}, -1},
                        {{I, J, K}, 1},
                        {{I, L, K}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, SizeIs(12));
  EXPECT_THAT(triangles, SizeIs(14));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{4, 2}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{2.5f, 2}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{5.5f, 2}));
}

TEST(TriangulateTest, HolesStacked) {
  //  D-----------C
  //  |     J     |
  //  |    / \    |
  //  |   I   K   |
  //  |    \ /    |
  //  |     L     |
  //  |           |
  //  |     F     |
  //  |    / \    |
  //  |   E   G   |
  //  |    \ /    |
  //  |     H     |
  //  A-----------B
  Point A{0, 0}, B{4, 0}, C{4, 8}, D{0, 8};
  Point E{1, 2}, F{2, 3}, G{3, 2}, H{2, 1};
  Point I{1, 6}, J{2, 7}, K{3, 6}, L{2, 5};
  ShapeOutline outline({{{A, B, C}, 1},
                        {{A, D, C}, -1},
                        {{E, F, G}, 1},
                        {{E, H, G}, -1},
                        {{I, J, K}, 1},
                        {{I, L, K}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, SizeIs(12));
  EXPECT_THAT(triangles, SizeIs(14));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{2, 4}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{2, 2}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{2, 6}));
}

TEST(TriangulateTest, DisjointComponents) {
  //      C           F
  //     / \         / \
  //    A---B       D---E
  Point A{0, 0}, B{2, 0}, C{1, 1}, D{3, 0}, E{5, 0}, F{4, 1};
  ShapeOutline outline(
      {{{A, B}, 1}, {{A, C, B}, -1}, {{D, E}, 1}, {{D, F, E}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, D, E, F));
  EXPECT_THAT(triangles, SizeIs(2));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
}

TEST(TriangulateTest, PinchPoint) {
  //         C     F
  //        / \   / \
  //       /   \ /   \
  //      A     P     E
  //       \   / \   /
  //        \ /   \ /
  //         B     D
  // Both chains pass through P.
  Point A{0, 0}, B{1, -1}, C{1, 1}, P{2, 0}, D{3, -1}, E{4, 0}, F{3, 1};
  ShapeOutline outline({{{A, B, P, D, E}, 1}, {{A, C, P, F, E}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, B, C, P, D, E, F));
  EXPECT_THAT(triangles, SizeIs(4));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 0}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{3, 0}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{2, 0.5f}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{2, -0.5f}));
}

TEST(TriangulateTest, HoleTouchingBoundaryAtVertex) {
  //  D-----------------C
  //  |                 |
  //  |       H         |
  //  |      /  \       |
  //  |     /     \     |
  //  |    /       J    |
  //  |   /  _ - ‾      |
  //  A--M--------------B

  Point A{0, 0}, M{2, 0}, B{6, 0}, C{6, 4}, D{0, 4}, H{3, 2}, J{4, 1};
  ShapeOutline outline(
      {{{A, M, B, C}, 1}, {{A, D, C}, -1}, {{M, H, J}, 1}, {{M, J}, -1}});

  auto [points, triangles] = Triangulate(outline);
  EXPECT_THAT(points, UnorderedElementsAre(A, M, B, C, D, H, J));
  EXPECT_TRUE(HasValidIndices(points, triangles));
  EXPECT_TRUE(AreCounterClockwise(points, triangles));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{1, 1}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{5, 0.5f}));
  EXPECT_TRUE(ContainsPoint(points, triangles, Point{3, 3}));
  EXPECT_FALSE(ContainsPoint(points, triangles, Point{3, 1}));
}

}  // namespace
}  // namespace ink::geometry_internal
