/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_RTREE_CREATOR_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_RTREE_CREATOR_H_

#include <memory>

#include "third_party/absl/memory/memory.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/spatial/mesh_rtree.h"
#include "ink/engine/geometry/spatial/spatial_index.h"
#include "ink/engine/geometry/spatial/spatial_index_factory.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {
namespace spatial {

class RTreeCreator : public Task {
 public:
  RTreeCreator(std::weak_ptr<SpatialIndexFactory> weak_factory, ElementId id,
               const OptimizedMesh& opt_mesh)
      : weak_factory_(std::move(weak_factory)), id_(id), opt_mesh_(opt_mesh) {}

  bool RequiresPreExecute() const override { return false; }
  void PreExecute() override {}

  void Execute() override {
    if (!weak_factory_.expired())
      index_ = absl::make_unique<MeshRTree>(opt_mesh_);
  }

  void OnPostExecute() override {
    std::shared_ptr<SpatialIndexFactory> factory = weak_factory_.lock();
    if (factory) factory->RegisterElementSpatialIndex(id_, std::move(index_));
  }

 private:
  std::weak_ptr<SpatialIndexFactory> weak_factory_;
  ElementId id_;
  OptimizedMesh opt_mesh_;
  std::unique_ptr<SpatialIndex> index_;
};

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_RTREE_CREATOR_H_
