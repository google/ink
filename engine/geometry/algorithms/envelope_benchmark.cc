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

#include "ink/engine/geometry/algorithms/envelope.h"

#include "testing/base/public/benchmark.h"
#include "testing/base/public/gunit.h"
#include "ink/engine/geometry/mesh/vertex.h"

namespace ink {
namespace geometry {
namespace {

template <typename PointType>
static void BM_PointEnvelope(benchmark::State &state) {
  int n_points = state.range(0);
  std::vector<PointType> points;
  points.reserve(n_points);
  for (int i = 0; i < n_points; ++i) points.emplace_back(i, i);
  while (state.KeepRunning()) {
    Envelope(points);
  }
}
BENCHMARK_TEMPLATE(BM_PointEnvelope, glm::vec2)->Range(2, 4096);
BENCHMARK_TEMPLATE(BM_PointEnvelope, Vertex)->Range(2, 4096);

}  // namespace
}  // namespace geometry
}  // namespace ink
