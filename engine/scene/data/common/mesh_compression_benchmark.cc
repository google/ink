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

#include <string>
#include <vector>

#include "file/base/helpers.h"
#include "file/base/options.h"
#include "file/base/path.h"
#include "testing/base/public/benchmark.h"
#include "testing/base/public/gunit.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/scene/data/common/draco_serializer.h"
#include "ink/engine/scene/data/common/openctm_serializer.h"
#include "ink/proto/document_portable_proto.pb.h"

namespace ink {
namespace mesh {

constexpr char kTestDataPath[] =
    "google3/ink/engine/scene/data/common/testdata/";

proto::Snapshot ReadTestSnapshot(absl::string_view basename) {
  auto path = file::JoinPath(FLAGS_test_srcdir, kTestDataPath,
                             absl::StrCat(basename, ".ink"));
  string buf;
  QCHECK_OK(file::GetContents(path, &buf, file::Defaults()));
  proto::Snapshot result;
  QCHECK(result.ParseFromString(buf));
  return result;
}

std::vector<proto::LOD> ReadDracoLods(absl::string_view basename) {
  auto snap = ReadTestSnapshot(basename);
  std::vector<proto::LOD> result;
  result.reserve(snap.element_size());
  for (const auto& element : snap.element()) {
    result.push_back(element.element().stroke().lod(0));
  }
  return result;
}

std::vector<proto::LOD> ReadOpenCtmLods(absl::string_view basename) {
  auto lods = ReadDracoLods(basename);
  DracoReader reader;
  OpenCtmWriter writer(VertFormat::x12y12);
  for (int i = 0; i < lods.size(); i++) {
    Mesh mesh;
    QCHECK_OK(reader.LodToMesh(lods[i], ShaderType::SingleColorShader,
                               0xFF000000, &mesh));
    lods[i].clear_draco_blob();
    OptimizedMesh optmesh(ShaderType::SingleColorShader, mesh);
    QCHECK_OK(writer.MeshToLod(optmesh, &lods[i]));
  }
  return lods;
}

std::vector<OptimizedMesh> ReadMeshes(absl::string_view basename) {
  auto lods = ReadDracoLods(basename);
  DracoReader reader;
  std::vector<OptimizedMesh> result;
  result.reserve(lods.size());
  for (const auto& lod : lods) {
    Mesh mesh;
    QCHECK_OK(reader.LodToMesh(lod, ShaderType::SingleColorShader, 0xFF000000,
                               &mesh));
    result.emplace_back(ShaderType::SingleColorShader, mesh);
  }
  return result;
}

void BM_ReadTextMeshesOpenCtm(benchmark::State& state) {
  auto lods = ReadOpenCtmLods("textish-draco");
  OpenCtmReader reader;
  for (auto _ : state) {
    for (const auto& lod : lods) {
      Mesh mesh;
      testing::DoNotOptimize(reader.LodToMesh(
          lod, ShaderType::SingleColorShader, 0xFF000000, &mesh));
    }
  }
}
BENCHMARK(BM_ReadTextMeshesOpenCtm);

void BM_ReadPaintingMeshesOpenCtm(benchmark::State& state) {
  auto lods = ReadOpenCtmLods("paintish-draco");
  OpenCtmReader reader;
  for (auto _ : state) {
    for (const auto& lod : lods) {
      Mesh mesh;
      testing::DoNotOptimize(reader.LodToMesh(
          lod, ShaderType::SingleColorShader, 0xFF000000, &mesh));
    }
  }
}
BENCHMARK(BM_ReadPaintingMeshesOpenCtm);

void BM_ReadTextMeshesDraco(benchmark::State& state) {
  auto lods = ReadDracoLods("textish-draco");
  DracoReader reader;
  for (auto _ : state) {
    for (const auto& lod : lods) {
      Mesh mesh;
      testing::DoNotOptimize(reader.LodToMesh(
          lod, ShaderType::SingleColorShader, 0xFF000000, &mesh));
    }
  }
}
BENCHMARK(BM_ReadTextMeshesDraco);

void BM_ReadPaintingMeshesDraco(benchmark::State& state) {
  auto lods = ReadDracoLods("paintish-draco");
  DracoReader reader;
  for (auto _ : state) {
    for (const auto& lod : lods) {
      Mesh mesh;
      testing::DoNotOptimize(reader.LodToMesh(
          lod, ShaderType::SingleColorShader, 0xFF000000, &mesh));
    }
  }
}
BENCHMARK(BM_ReadPaintingMeshesDraco);

void BM_WriteTextMeshesOpenCtm(benchmark::State& state) {
  auto meshes = ReadMeshes("textish-draco");
  OpenCtmWriter writer(VertFormat::x12y12);
  uint32_t total_size{0};
  bool calculated = false;
  for (auto _ : state) {
    for (const auto& mesh : meshes) {
      proto::LOD lod;
      testing::DoNotOptimize(writer.MeshToLod(mesh, &lod));
      if (!calculated) total_size += lod.ctm_blob().size();
    }
    if (!calculated) {
      LOG(INFO) << "text mesh total size OpenCTM: " << total_size;
    }
    calculated = true;
  }
}
BENCHMARK(BM_WriteTextMeshesOpenCtm);

void BM_WritePaintingMeshesOpenCtm(benchmark::State& state) {
  auto meshes = ReadMeshes("paintish-draco");
  OpenCtmWriter writer(VertFormat::x12y12);
  uint32_t total_size{0};
  bool calculated = false;
  for (auto _ : state) {
    for (const auto& mesh : meshes) {
      proto::LOD lod;
      testing::DoNotOptimize(writer.MeshToLod(mesh, &lod));
      if (!calculated) total_size += lod.ctm_blob().size();
    }
    if (!calculated) {
      LOG(INFO) << "painting mesh total size OpenCTM: " << total_size;
    }
    calculated = true;
  }
}
BENCHMARK(BM_WritePaintingMeshesOpenCtm);

void BM_WriteTextMeshesDraco(benchmark::State& state) {
  auto meshes = ReadMeshes("textish-draco");
  DracoWriter writer(VertFormat::x12y12);
  uint32_t total_size{0};
  bool calculated = false;
  for (auto _ : state) {
    for (const auto& mesh : meshes) {
      proto::LOD lod;
      testing::DoNotOptimize(writer.MeshToLod(mesh, &lod));
      if (!calculated) total_size += lod.draco_blob().size();
    }
    if (!calculated) {
      LOG(INFO) << "text mesh total size Draco: " << total_size;
    }
    calculated = true;
  }
}
BENCHMARK(BM_WriteTextMeshesDraco);

void BM_WritePaintingMeshesDraco(benchmark::State& state) {
  auto meshes = ReadMeshes("paintish-draco");
  DracoWriter writer(VertFormat::x12y12, state.range(0));
  uint32_t total_size{0};
  bool calculated = false;
  for (auto _ : state) {
    for (const auto& mesh : meshes) {
      proto::LOD lod;
      testing::DoNotOptimize(writer.MeshToLod(mesh, &lod));
      if (!calculated) total_size += lod.draco_blob().size();
    }
    if (!calculated) {
      LOG(INFO) << "painting mesh total size Draco: " << total_size;
    }
    calculated = true;
  }
}
BENCHMARK(BM_WritePaintingMeshesDraco)
    ->Arg(0)
    ->Arg(2)
    ->Arg(4)
    ->Arg(5)
    ->Arg(6)
    ->Arg(8)
    ->Arg(10);

}  // namespace mesh
}  // namespace ink
