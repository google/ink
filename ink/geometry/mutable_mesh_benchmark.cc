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
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_test_helpers.h"
#include "ink/geometry/mutable_mesh.h"
#include "ink/geometry/point.h"
#include "ink/types/small_array.h"

namespace ink {
namespace {

using ::benchmark::internal::Benchmark;

void BM_AsMesh(benchmark::State& state) {
  state.SetLabel(absl::StrFormat("mesh: %s", kTestMeshFiles[state.range(0)]));

  absl::StatusOr<Mesh> mesh = LoadMesh(kTestMeshFiles[state.range(0)]);
  ABSL_CHECK_OK(mesh);
  MutableMesh mutable_mesh = MutableMesh::FromMesh(*mesh);

  for (auto s : state) {
    auto immutable_mesh = mutable_mesh.AsMeshes();
    ABSL_CHECK_OK(immutable_mesh);
    benchmark::DoNotOptimize(immutable_mesh);
  }
}
BENCHMARK(BM_AsMesh)->DenseRange(0, kTestMeshFiles.size() - 1);

void BM_GetVerticesAndTriangles(benchmark::State& state) {
  state.SetLabel(absl::StrFormat("mesh: %s", kTestMeshFiles[state.range(0)]));

  auto mesh = LoadMesh(kTestMeshFiles[state.range(0)]);
  ABSL_CHECK_OK(mesh);
  MutableMesh mutable_mesh = MutableMesh::FromMesh(*mesh);

  int num_attributes = mutable_mesh.Format().Attributes().size();
  int num_vertices = mutable_mesh.VertexCount();
  int num_triangles = mutable_mesh.TriangleCount();

  for (auto s : state) {
    for (int i = 0; i < num_vertices; ++i) {
      for (int j = 0; j < num_attributes; ++j) {
        benchmark::DoNotOptimize(mutable_mesh.FloatVertexAttribute(i, j));
      }
    }

    for (int i = 0; i < num_triangles; ++i) {
      benchmark::DoNotOptimize(mutable_mesh.TriangleIndices(i));
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
  int num_attributes = mesh->Format().Attributes().size();
  int num_vertices = mesh->VertexCount();
  int num_triangles = mesh->TriangleCount();
  std::vector<std::array<uint32_t, 3>> triangles(num_triangles);
  for (int i = 0; i < num_triangles; ++i) {
    triangles[i] = mesh->TriangleIndices(i);
  }
  std::vector<SmallArray<float, 4>> attributes(num_vertices * num_attributes);
  for (int v = 0; v < num_vertices; ++v) {
    for (int a = 0; a < num_attributes; ++a) {
      attributes[v * num_attributes + a] = mesh->FloatVertexAttribute(v, a);
    }
  }

  for (auto s : state) {
    MutableMesh mutable_mesh(mesh->Format());

    for (int v = 0; v < num_vertices; ++v) {
      mutable_mesh.AppendVertex(kOrigin);
    }

    for (int v = 0; v < num_vertices; ++v) {
      for (int a = 0; a < num_attributes; ++a) {
        mutable_mesh.SetFloatVertexAttribute(
            v, a, attributes[v * num_attributes + a]);
      }
    }

    for (int i = 0; i < num_triangles; ++i) {
      mutable_mesh.AppendTriangleIndices(triangles[i]);
    }
  }
}
BENCHMARK(BM_CreateMeshIncrementally)->DenseRange(0, kTestMeshFiles.size() - 1);

}  // namespace
}  // namespace ink
