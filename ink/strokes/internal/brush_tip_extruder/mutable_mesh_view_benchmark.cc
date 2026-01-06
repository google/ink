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

#include <array>
#include <cstdint>
#include <vector>

#include "benchmark/benchmark.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ink/geometry/mesh_test_helpers.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/strokes/internal/brush_tip_extruder/extruded_vertex.h"
#include "ink/strokes/internal/brush_tip_extruder/mutable_mesh_view.h"

namespace ink::brush_tip_extruder_internal {
namespace {

using ::benchmark::internal::Benchmark;

void BM_GetVerticesAndTriangles(benchmark::State& state) {
  state.SetLabel(absl::StrFormat("mesh: %s", kTestMeshFiles[state.range(0)]));

  auto mesh = LoadMesh(kTestMeshFiles[state.range(0)]);
  ABSL_CHECK_OK(mesh);
  MutableMesh mutable_mesh = MutableMesh::FromMesh(*mesh);
  MutableMeshView mutable_mesh_view(mutable_mesh);

  int num_vertices = mutable_mesh.VertexCount();
  int num_triangles = mutable_mesh.TriangleCount();

  for (auto s : state) {
    for (int i = 0; i < num_vertices; ++i) {
      benchmark::DoNotOptimize(mutable_mesh_view.GetVertex(i));
    }

    for (int i = 0; i < num_triangles; ++i) {
      benchmark::DoNotOptimize(mutable_mesh_view.GetTriangleIndices(i));
    }
  }
}
BENCHMARK(BM_GetVerticesAndTriangles)->DenseRange(0, kTestMeshFiles.size() - 1);

void BM_CreateMeshIncrementally(benchmark::State& state) {
  state.SetLabel(absl::StrFormat("mesh: %s", kTestMeshFiles[state.range(0)]));
  auto mesh = LoadMesh(kTestMeshFiles[state.range(0)]);
  ABSL_CHECK_OK(mesh);

  // First, read the vertices and triangles from the mesh before starting
  // the benchmark, so that the benchmark loop only measures the runtime of mesh
  // creation.
  MutableMesh mutable_mesh = MutableMesh::FromMesh(*mesh);
  MutableMeshView mutable_mesh_view(mutable_mesh);
  int num_vertices = mutable_mesh_view.VertexCount();
  int num_triangles = mutable_mesh_view.TriangleCount();
  std::vector<ExtrudedVertex> vertices(num_vertices);
  for (int i = 0; i < num_vertices; ++i) {
    vertices[i] = mutable_mesh_view.GetVertex(i);
  }
  std::vector<std::array<uint32_t, 3>> triangles(num_triangles);
  for (int i = 0; i < num_triangles; ++i) {
    triangles[i] = mutable_mesh_view.GetTriangleIndices(i);
  }

  for (auto s : state) {
    MutableMesh mutable_mesh(mesh->Format());
    MutableMeshView mutable_mesh_view(mutable_mesh);

    for (int i = 0; i < num_vertices; ++i) {
      mutable_mesh_view.AppendVertex(vertices[i]);
    }

    for (int i = 0; i < num_triangles; ++i) {
      mutable_mesh_view.AppendTriangleIndices(triangles[i]);
    }
  }
}
BENCHMARK(BM_CreateMeshIncrementally)->DenseRange(0, kTestMeshFiles.size() - 1);

}  // namespace
}  // namespace ink::brush_tip_extruder_internal
