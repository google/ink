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

#include "ink/engine/rendering/scene_drawable.h"

#include <algorithm>

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/rendering/gl_managers/scissor.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

std::shared_ptr<MeshSceneDrawable> MeshSceneDrawable::AddToScene(
    const ElementId& id, const GroupId& group_id, const Mesh& mesh,
    std::shared_ptr<SceneGraph> graph,
    std::shared_ptr<GLResourceManager> gl_resources,
    std::shared_ptr<FrameState> frame_state) {
  ASSERT(id != kInvalidElementId);
  SLOG(SLOG_DATA_FLOW, "scene drawable constructed to render id $0", id);
  auto res = std::shared_ptr<MeshSceneDrawable>(new MeshSceneDrawable(
      id, group_id, mesh, graph, gl_resources, frame_state));
  graph->AddDrawable(res);
  graph->AddListener(res.get());
  graph->RegisterForUpdates(res.get());
  return res;
}

MeshSceneDrawable::MeshSceneDrawable(
    const ElementId& id, const GroupId& group_id, const Mesh& mesh,
    std::shared_ptr<SceneGraph> graph,
    std::shared_ptr<GLResourceManager> gl_resources,
    std::shared_ptr<FrameState> frame_state)
    : graph_weak_ptr_(graph),
      gl_resources_(gl_resources),
      frame_state_(frame_state),
      id_(id),
      group_id_(group_id),
      mesh_(mesh),
      renderer_(gl_resources),
      earliest_remove_time_(0) {
  SLOG(SLOG_DATA_FLOW, "creating scene drawable $0, setting to invisible", id_);
  gl_resources->mesh_vbo_provider->GenVBOs(&mesh_, GL_STATIC_DRAW);
  if (mesh_.shader_metadata.IsAnimated()) {
    frame_lock_ =
        frame_state->AcquireFramerateLock(60, "SceneDrawable animation");
    DurationS animation_duration = 0;
    for (const auto& v : mesh_.verts) {
      animation_duration =
          std::max(animation_duration, DurationS(v.position_timings.y));
      animation_duration =
          std::max(animation_duration, DurationS(v.color_timings.y));
      animation_duration =
          std::max(animation_duration, DurationS(v.texture_timings.y));
    }
    earliest_remove_time_ =
        animation_duration + mesh.shader_metadata.InitTime();
    SLOG(SLOG_DATA_FLOW,
         "drawable $0 is animated, setting target removal at $1 seconds from "
         "now",
         id_,
         static_cast<double>(earliest_remove_time_ -
                             frame_state_->GetFrameTime()));
  }
  graph->SetElementRenderedByMain(id_, false);
}

MeshSceneDrawable::~MeshSceneDrawable() {
  SLOG(SLOG_DATA_FLOW, "scene drawable $0 removed. Resetting visibility.", id_);

  auto graph = graph_weak_ptr_.lock();
  if (graph) {
    graph->SetElementRenderedByMain(&id_, &id_ + 1, true);
  }
}

void MeshSceneDrawable::Draw(const Camera& cam, FrameTimeS draw_time) const {
  std::unique_ptr<Scissor> scissor;
  if (group_id_ != kInvalidElementId) {
    auto graph = graph_weak_ptr_.lock();
    if (graph->IsClippableGroup(group_id_)) {
      auto bounds = graph->Mbr({group_id_});
      scissor = absl::make_unique<Scissor>(gl_resources_->gl);
      scissor->SetScissor(cam, bounds, CoordType::kWorld);
    }
  }
  renderer_.Draw(cam, draw_time, mesh_);
}

void MeshSceneDrawable::Update(const Camera& cam) {
  if (frame_state_->GetFrameTime() >= earliest_remove_time_) {
    frame_lock_.reset();

    auto graph = graph_weak_ptr_.lock();
    if (graph && graph->ElementExists(id_)) {
      Remove();
    }
  }
}

void MeshSceneDrawable::Remove() {
  SLOG(SLOG_DATA_FLOW, "removing scene drawable id $0", id_);

  auto graph = graph_weak_ptr_.lock();
  if (graph) {
    graph->RemoveDrawable(this);  // Probably will call dtor!
  } else {
    SLOG(SLOG_WARNING, "Remove after scenegraph cleaned up");
  }
}

void MeshSceneDrawable::OnElementAdded(SceneGraph* graph, ElementId id) {
  // If we finished animating before a corresponding SceneElementAdder has added
  // itself to the scene graph, this drawable continues to be drawn. Once the
  // element is added we need to trigger another Update, which will in turn
  // remove this Drawable from the SceneGraph, which will trigger the destructor
  // which then makes the newly added Element visible.
  Update(Camera());
}

void MeshSceneDrawable::OnElementsRemoved(
    SceneGraph* graph, const std::vector<SceneGraphRemoval>& removed_elements) {
  auto id_equal = [this](const SceneGraphRemoval& r) { return r.id == id_; };
  if (std::any_of(removed_elements.begin(), removed_elements.end(), id_equal)) {
    Remove();
  }
}
}  // namespace ink
