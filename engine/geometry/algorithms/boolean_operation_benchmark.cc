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

#include "testing/base/public/benchmark.h"
#include "testing/base/public/gunit.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/polygon.h"
#include "ink/engine/math_defines.h"

namespace ink {
namespace geometry {
namespace {

struct IntersectionOp {
  void operator()(const Polygon &lhs, const Polygon &rhs) const {
    Intersection(lhs, rhs);
  }
};

struct DifferenceOp {
  void operator()(const Polygon &lhs, const Polygon &rhs) const {
    Difference(lhs, rhs);
  }
};

std::vector<glm::vec2> MakeCircle(glm::vec2 center, float radius,
                                  int n_points) {
  // PointsOnCircle() always includes both endpoints, so we adjust the end angle
  // to be just short of a full circle.
  return PointsOnCircle(center, radius, n_points, 0,
                        M_TAU * static_cast<float>(n_points - 1) / n_points);
}

template <typename Operation>
static void BM_DisjointPolygons(benchmark::State &state) {
  int n_points = state.range(0);
  Polygon lhs(MakeCircle({0, 0}, 40, n_points));
  Polygon rhs(MakeCircle({100, 0}, 40, n_points));
  Operation op;
  for (auto _ : state) op(lhs, rhs);
}
BENCHMARK(BM_DisjointPolygons<IntersectionOp>)->Range(4, 1024);
BENCHMARK(BM_DisjointPolygons<DifferenceOp>)->Range(4, 1024);

template <typename Operation>
static void BM_LeftInsideRight(benchmark::State &state) {
  int n_points = state.range(0);
  Polygon lhs(MakeCircle({0, 0}, 50, n_points));
  Polygon rhs(MakeCircle({0, 0}, 100, n_points));
  Operation op;
  for (auto _ : state) op(lhs, rhs);
}
BENCHMARK(BM_LeftInsideRight<IntersectionOp>)->Range(4, 1024);
BENCHMARK(BM_LeftInsideRight<DifferenceOp>)->Range(4, 1024);

template <typename Operation>
static void BM_RightInsideLeft(benchmark::State &state) {
  int n_points = state.range(0);
  Polygon lhs(MakeCircle({0, 0}, 100, n_points));
  Polygon rhs(MakeCircle({0, 0}, 50, n_points));
  Operation op;
  for (auto _ : state) op(lhs, rhs);
}
BENCHMARK(BM_RightInsideLeft<IntersectionOp>)->Range(4, 1024);
BENCHMARK(BM_RightInsideLeft<DifferenceOp>)->Range(4, 1024);

template <typename Operation>
static void BM_SimpleCase(benchmark::State &state) {
  int n_points = state.range(0);
  Polygon lhs(MakeCircle({0, 0}, 100, n_points));
  Polygon rhs(MakeCircle({100, 0}, 100, n_points));
  Operation op;
  for (auto _ : state) op(lhs, rhs);
}
BENCHMARK(BM_SimpleCase<IntersectionOp>)->Range(4, 1024);
BENCHMARK(BM_SimpleCase<DifferenceOp>)->Range(4, 1024);

template <typename Operation>
static void BM_ComplexCase(benchmark::State &state) {
  int n_points = state.range(0);
  std::vector<glm::vec2> points;
  points.reserve(n_points);
  for (int i = 0; i < n_points / 2; ++i) points.emplace_back(i, (i % 2));
  for (int i = 0; i < n_points / 2; ++i)
    points.emplace_back(points[n_points / 2 - 1 - i] + glm::vec2{0, .5});
  Polygon lhs(points);
  for (auto &p : points) p.y = 1.5 - p.y;
  std::reverse(points.begin(), points.end());
  Polygon rhs(points);
  Operation op;
  for (auto _ : state) op(lhs, rhs);
}
BENCHMARK(BM_ComplexCase<IntersectionOp>)->Range(4, 1024);
BENCHMARK(BM_ComplexCase<DifferenceOp>)->Range(4, 1024);

}  // namespace
}  // namespace geometry
}  // namespace ink
