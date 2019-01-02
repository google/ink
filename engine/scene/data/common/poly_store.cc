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

#include "ink/engine/scene/data/common/poly_store.h"

#include <cmath>

#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

PolyStore::PolyStore(std::shared_ptr<GLResourceManager> gl_resources,
                     std::shared_ptr<settings::Flags> flags)
    : gl_resources_(std::move(gl_resources)), flags_(std::move(flags)) {}

void PolyStore::Add(ElementId id, std::unique_ptr<OptimizedMesh> mesh) {
  if (id.Type() != ElementType::POLY) {
    SLOG(SLOG_WARNING, "Cannot store non-poly element in PolyStore (id: $0).",
         id);
    return;
  } else if (mesh == nullptr) {
    SLOG(SLOG_WARNING, "Cannot store null mesh in PolyStore (id: $0).", id);
    return;
  }
  ASSERT(id_to_mesh_.find(id) == id_to_mesh_.end());

  if (!flags_->GetFlag(settings::Flag::KeepMeshesInCpuMemory) ||
      flags_->GetFlag(settings::Flag::LowMemoryMode)) {
    gl_resources_->mesh_vbo_provider->EnsureOnlyInVBO(mesh.get(),
                                                      GL_STATIC_DRAW);
  } else if (!gl_resources_->mesh_vbo_provider->HasVBO(*mesh)) {
    gl_resources_->mesh_vbo_provider->GenVBO(mesh.get(), GL_STATIC_DRAW);
  }
  id_to_mesh_[id] = std::move(mesh);

  SLOG(SLOG_DATA_FLOW, "polystore adding element id:$0", id);
}

void PolyStore::Remove(ElementId id) {
  auto it = id_to_mesh_.find(id);
  if (it == id_to_mesh_.end()) {
    SLOG(SLOG_WARNING, "poly store couldn't find element $0 for removal", id);
    return;
  }
  id_to_mesh_.erase(id);
}

void PolyStore::OnMemoryWarning() {
  SLOG(SLOG_WARNING,
       "polystore received memory warning, but is unable to help");
}

S_WARN_UNUSED_RESULT bool PolyStore::Get(ElementId id,
                                         OptimizedMesh** mesh) const {
  *mesh = nullptr;
  auto it = id_to_mesh_.find(id);
  if (it == id_to_mesh_.end()) return false;
  ASSERT(it->second != nullptr);
  *mesh = it->second.get();
  return true;
}
}  // namespace ink
