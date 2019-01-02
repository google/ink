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

#include "testing/base/public/benchmark.h"
#include "testing/base/public/gunit.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/triangle.h"
#include "ink/engine/math_defines.h"

namespace ink {
namespace geometry {
namespace {

static void BM_SegmentIntersectHit(benchmark::State &state) {
  Segment seg0{{10, 10}, {20, 20}};
  Segment seg1{{10, 20}, {20, 10}};
  for (auto _ : state) Intersects(seg0, seg1);
}
BENCHMARK(BM_SegmentIntersectHit);

static void BM_SegmentIntersectMiss(benchmark::State &state) {
  Segment seg0{{10, 10}, {20, 20}};
  Segment seg1{{50, 20}, {70, 10}};
  for (auto _ : state) Intersects(seg0, seg1);
}
BENCHMARK(BM_SegmentIntersectMiss);

static void BM_SegmentIntersectCollinearMiss(benchmark::State &state) {
  Segment seg0{{5, 10}, {10, 15}};
  Segment seg1{{15, 20}, {20, 25}};
  for (auto _ : state) Intersects(seg0, seg1);
}
BENCHMARK(BM_SegmentIntersectCollinearMiss);

static void BM_SegmentIntersectOverlap(benchmark::State &state) {
  Segment seg0{{10, 10}, {20, 20}};
  Segment seg1{{15, 25}, {15, 25}};
  for (auto _ : state) Intersects(seg0, seg1);
}
BENCHMARK(BM_SegmentIntersectOverlap);

static void BM_SegmentIntersectEndpointHit(benchmark::State &state) {
  Segment seg0{{0, 0}, {20, 0}};
  Segment seg1{{10, 0}, {10, 10}};
  for (auto _ : state) Intersects(seg0, seg1);
}
BENCHMARK(BM_SegmentIntersectEndpointHit);

static void BM_SegmentIntersectEndpointMiss(benchmark::State &state) {
  Segment seg0{{0, 0}, {20, 0}};
  Segment seg1{{30, 0}, {10, 10}};
  for (auto _ : state) Intersects(seg0, seg1);
}
BENCHMARK(BM_SegmentIntersectEndpointMiss);

static void BM_TriangleIntersectHit(benchmark::State &state) {
  Triangle tri0(3, 10, 7, 2, 9, 5);
  Triangle tri1(3, 7, 6, 2, 8, 10);
  for (auto _ : state) Intersects(tri0, tri1);
}
BENCHMARK(BM_TriangleIntersectHit);

static void BM_TriangleIntersectMiss(benchmark::State &state) {
  Triangle tri0(3, 10, 7, 2, 9, 5);
  Triangle tri1(16, 13, 14, 10, 12, 12);
  for (auto _ : state) Intersects(tri0, tri1);
}
BENCHMARK(BM_TriangleIntersectMiss);

static void BM_TriangleIntersectFirstInside(benchmark::State &state) {
  Triangle tri0(3, 10, 7, 2, 9, 5);
  Triangle tri1(3, 1, 2, 12, 22, 4);
  for (auto _ : state) Intersects(tri0, tri1);
}
BENCHMARK(BM_TriangleIntersectFirstInside);

static void BM_TriangleIntersectSecondInside(benchmark::State &state) {
  Triangle tri0(7, 4, 8, 5, 6, 9);
  Triangle tri1(3, 10, 7, 2, 9, 5);
  for (auto _ : state) Intersects(tri0, tri1);
}
BENCHMARK(BM_TriangleIntersectSecondInside);

static void BM_PolygonIntersectionMiss(benchmark::State &state) {
  int n_points = state.range(0);
  std::vector<glm::vec2> lhs_points =
      PointsOnCircle({-120, 0}, 100, n_points, 0, M_TAU);
  std::vector<glm::vec2> rhs_points =
      PointsOnCircle({120, 0}, 100, n_points, 0, M_TAU);
  for (auto _ : state) {
    std::vector<PolygonIntersection> result;
    Intersection(Polygon(lhs_points), Polygon(rhs_points), &result);
  }
}
BENCHMARK(BM_PolygonIntersectionMiss)->Range(4, 1024);

static void BM_PolygonIntersectionFewHits(benchmark::State &state) {
  int n_points = state.range(0);
  std::vector<glm::vec2> lhs_points =
      PointsOnCircle({0, 0}, 100, n_points, 0, M_TAU);
  std::vector<glm::vec2> rhs_points;
  rhs_points.reserve(n_points);
  for (int i = 0; i < n_points; ++i) {
    float t = static_cast<float>(i) / (n_points - 1);
    rhs_points.emplace_back(2 * t - 1, std::cos(3 * M_PI * t));
  }
  for (auto _ : state) {
    std::vector<PolygonIntersection> result;
    Intersection(Polygon(lhs_points), Polygon(rhs_points), &result);
  }
}
BENCHMARK(BM_PolygonIntersectionFewHits)->Range(4, 1024);

static void BM_PolygonIntersectionManyHits(benchmark::State &state) {
  int n_points = state.range(0);
  std::vector<glm::vec2> lhs_points;
  std::vector<glm::vec2> rhs_points;
  lhs_points.reserve(n_points);
  rhs_points.reserve(n_points);
  for (int i = 0; i < n_points; ++i) {
    float t = static_cast<float>(i) / (n_points - 1);
    lhs_points.emplace_back(t, i % 2);
    rhs_points.emplace_back(i % 2, t);
  }
  for (auto _ : state) {
    std::vector<PolygonIntersection> result;
    Intersection(Polygon(lhs_points), Polygon(rhs_points), &result);
  }
}
BENCHMARK(BM_PolygonIntersectionManyHits)->Range(4, 1024);

}  // namespace
}  // namespace geometry
}  // namespace ink
