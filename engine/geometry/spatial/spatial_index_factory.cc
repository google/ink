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

#include "ink/engine/geometry/spatial/spatial_index_factory.h"

#include <memory>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/spatial/mesh_rtree.h"
#include "ink/engine/geometry/spatial/rtree_creator.h"
#include "ink/engine/geometry/spatial/spatial_index.h"
#include "ink/engine/geometry/spatial/texture_rtree_creator.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/data/common/serialized_element.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/graph/scene_graph_listener.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/settings/flags.h"

namespace ink {
namespace spatial {
namespace {
std::shared_ptr<SpatialIndex> MakeRectIndex(ShaderType type) {
  float max_obj_coord = PackedVertList::GetMaxCoordinateForFormat(
      OptimizedMesh::VertexFormat(type));
  Mesh rect_mesh;
  MakeRectangleMesh(&rect_mesh, Rect(0, 0, max_obj_coord, max_obj_coord));
  return std::unique_ptr<SpatialIndex>(
      new MeshRTree(OptimizedMesh(type, rect_mesh)));
}
}  // namespace

SpatialIndexFactory::SpatialIndexFactory(
    std::shared_ptr<settings::Flags> flags,
    std::shared_ptr<ITaskRunner> task_runner,
    std::shared_ptr<GLResourceManager> gl_resources)
    : flags_(std::move(flags)),
      task_runner_(std::move(task_runner)),
      gl_resources_(std::move(gl_resources)),
      scene_graph_(nullptr) {
  gl_resources_->texture_manager->AddListener(this);
}

SpatialIndexFactory::~SpatialIndexFactory() {
  gl_resources_->texture_manager->RemoveListener(this);
}

std::shared_ptr<SpatialIndex> SpatialIndexFactory::CreateSpatialIndex(
    const ProcessedElement& element) {
  ASSERT(scene_graph_ != nullptr);
  const OptimizedMesh& mesh = *element.mesh;

  if (mesh.texture) {
    Texture* texture = nullptr;
    if (element.attributes.is_sticker &&
        gl_resources_->texture_manager->GetTexture(*mesh.texture, &texture) &&
        texture->UseForHitTesting()) {
      auto iter = texture_uri_to_spatial_index_.find(mesh.texture->uri);
      if (iter != texture_uri_to_spatial_index_.end()) return iter->second;
    }
  }

  if (flags_->GetFlag(settings::Flag::LowMemoryMode)) {
    return MakeRectIndex(mesh.type);
  } else if (mesh.idx.size() <= 6) {
    // The mesh has only a couple of triangles, just create it now.
    return std::make_shared<MeshRTree>(mesh);
  } else {
    task_runner_->PushTask(
        absl::make_unique<RTreeCreator>(shared_from_this(), element.id, mesh));
    return MakeRectIndex(mesh.type);
  }
}

void SpatialIndexFactory::OnTextureLoaded(const TextureInfo& info) {
  if (flags_->GetFlag(settings::Flag::LowMemoryMode)) {
    return;
  }
  Texture* texture;
  EXPECT(gl_resources_->texture_manager->GetTexture(info, &texture));
  if (texture->UseForHitTesting())
    task_runner_->PushTask(absl::make_unique<TextureRTreeCreator>(
        shared_from_this(), info.uri, *texture));
}

void SpatialIndexFactory::OnTextureEvicted(const TextureInfo& info) {
  if (flags_->GetFlag(settings::Flag::LowMemoryMode)) {
    return;
  }
  texture_uri_to_spatial_index_.erase(info.uri);
  ReplaceTextureSpatialIndex(info.uri,
                             MakeRectIndex(ShaderType::TexturedVertShader));
}

void SpatialIndexFactory::RegisterElementSpatialIndex(
    ElementId id, std::shared_ptr<SpatialIndex> index) {
  ASSERT(scene_graph_ != nullptr);
  scene_graph_->SetSpatialIndex(id, std::move(index));
}

void SpatialIndexFactory::RegisterTextureSpatialIndex(
    const std::string& texture_uri, std::shared_ptr<SpatialIndex> index) {
  if (flags_->GetFlag(settings::Flag::LowMemoryMode)) {
    return;
  }
  texture_uri_to_spatial_index_[texture_uri] = index;
  ReplaceTextureSpatialIndex(texture_uri, index);
}

void SpatialIndexFactory::ReplaceTextureSpatialIndex(
    const std::string& texture_uri, std::shared_ptr<SpatialIndex> index) {
  ASSERT(scene_graph_ != nullptr);
  std::vector<ElementId> elements;
  scene_graph_->ElementsInScene(std::back_inserter(elements));
  elements.erase(std::remove_if(elements.begin(), elements.end(),
                                [texture_uri, this](ElementId id) {
                                  std::string uri;
                                  if (scene_graph_->GetTextureUri(id, &uri)) {
                                    return texture_uri != uri;
                                  } else {
                                    return true;
                                  }
                                }),
                 elements.end());

  std::vector<std::shared_ptr<SpatialIndex>> indices(elements.size(), index);
  scene_graph_->SetSpatialIndices(elements.begin(), elements.end(),
                                  indices.begin(), indices.end());
}

}  // namespace spatial
}  // namespace ink
