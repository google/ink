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

#ifndef INK_ENGINE_RENDERING_SCENE_DRAWABLE_H_
#define INK_ENGINE_RENDERING_SCENE_DRAWABLE_H_

#include <memory>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/graph/scene_graph_listener.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/updatable.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// Like a drawable, but knows about scenegraph obj lifetime.
//
// Modifies / tracks SceneGraph visibility to keep only one copy drawing at a
// time.
//
// Removes itself from the scene when:
//   - All mesh animations are complete
//   - And the scene has data for this element
//     --or--
//   - We see an explicit remove for the stroke
class MeshSceneDrawable : public SceneGraphListener,
                          public IDrawable,
                          public UpdateListener {
 public:
  // Constructs a MeshSceneDrawable, taking drawing ownership of id "uuid"
  // from the scene
  static std::shared_ptr<MeshSceneDrawable> AddToScene(
      const ElementId& id, const GroupId& group_id, const Mesh& mesh,
      std::shared_ptr<SceneGraph> graph,
      std::shared_ptr<GLResourceManager> gl_resources,
      std::shared_ptr<FrameState> frame_state);
  ~MeshSceneDrawable() override;

  void Draw(const Camera& cam,
            FrameTimeS draw_time) const override;  // IDrawable
  void Update(const Camera& cam) override;      // IUpdatable

  // SceneGraphListener
  void OnElementAdded(SceneGraph* graph, ElementId id) override;
  void OnElementsRemoved(
      SceneGraph* graph,
      const std::vector<SceneGraphRemoval>& removed_elements) override;
  void OnElementsMutated(
      SceneGraph* graph,
      const std::vector<ElementMutationData>& mutation_data) override {}

 protected:
  // The following are protected as they should not be called by regular
  // users of the API (who should construct this object through AddToScene
  // above), but is useful to be exposed to tests.
  MeshSceneDrawable(const ElementId& id, const GroupId& group_id,
                    const Mesh& mesh, std::shared_ptr<SceneGraph> graph,
                    std::shared_ptr<GLResourceManager> gl_resources,
                    std::shared_ptr<FrameState> frame_state);

  // Removes this drawable from the scene.
  // WARNING - should be viewed as calling the dtor as the scenegraph may hold
  // the only ref to this instance
  void Remove();

 private:
  // Since SceneGraph holds a shared_ptr to this object, we hold only a weak_ptr
  // to the SceneGraph here.
  std::weak_ptr<SceneGraph> graph_weak_ptr_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<FrameState> frame_state_;
  const ElementId id_;
  const GroupId group_id_;
  Mesh mesh_;
  const MeshRenderer renderer_;
  FrameTimeS earliest_remove_time_;
  std::unique_ptr<FramerateLock> frame_lock_;
};

}  // namespace ink
#endif  // INK_ENGINE_RENDERING_SCENE_DRAWABLE_H_
