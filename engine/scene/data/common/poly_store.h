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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_POLY_STORE_H_
#define INK_ENGINE_SCENE_DATA_COMMON_POLY_STORE_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/settings/flags.h"

namespace ink {

class PolyStore {
 public:
  using SharedDeps = service::Dependencies<GLResourceManager, settings::Flags>;

  PolyStore(std::shared_ptr<GLResourceManager> gl_resources,
            std::shared_ptr<settings::Flags> flags);

  void Add(ElementId id, std::unique_ptr<OptimizedMesh> mesh);
  void Remove(ElementId id);

  S_WARN_UNUSED_RESULT bool Get(ElementId id, OptimizedMesh** mesh) const;

  void OnMemoryWarning();

 private:
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<settings::Flags> flags_;

  std::unordered_map<ElementId, std::unique_ptr<OptimizedMesh>, ElementIdHasher>
      id_to_mesh_;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_DATA_COMMON_POLY_STORE_H_
