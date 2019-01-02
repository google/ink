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

#include "ink/engine/geometry/algorithms/convex_hull.h"

#include <random>
#include <vector>

#include "testing/base/public/benchmark.h"
#include "testing/base/public/gunit.h"
#include "third_party/glm/glm/glm.hpp"

namespace ink {
namespace geometry {
namespace {

static constexpr float kRandomSeed = 12345.6789f;

void BM_ConvexHull(benchmark::State &state) {
  int n_points = state.range(0);

  std::mt19937 rng(kRandomSeed);
  std::uniform_real_distribution<float> d(-50, 50);

  std::vector<glm::vec2> points;
  points.reserve(n_points);
  std::vector<glm::vec2> output;
  for (int i = 0; i < n_points; ++i) points.emplace_back(d(rng), d(rng));
  while (state.KeepRunning()) {
    ConvexHull(points);
  }
}
BENCHMARK(BM_ConvexHull)->Range(16, 4096);

}  // namespace
}  // namespace geometry
}  // namespace ink
