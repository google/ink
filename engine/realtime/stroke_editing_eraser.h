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

#ifndef INK_ENGINE_REALTIME_STROKE_EDITING_ERASER_H_
#define INK_ENGINE_REALTIME_STROKE_EDITING_ERASER_H_

#include <memory>
#include <queue>
#include <string>

#include "third_party/absl/synchronization/mutex.h"
#include "third_party/absl/types/optional.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh_splitter.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/input/input_modeler.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/realtime/line_builder.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/realtime/modifiers/line_modifier_factory.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
namespace tools {

// Eraser that removes only the parts of the element that it touches.
class StrokeEditingEraser : public Tool {
 public:
  explicit StrokeEditingEraser(const service::UncheckedRegistry& registry);

  void Draw(const Camera& camera, FrameTimeS draw_time) const override;
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& camera) override;
  std::string ToString() const override { return "<StrokeEditingEraser>"; }
  void SetBrushSize(BrushParams::BrushSize size);

 private:
  struct ElementData {
    ShaderType shader_type = ShaderType::NoShader;
    ElementAttributes attributes;
    Mesh mesh;
    std::unique_ptr<MeshSplitter> splitter;
    ElementId new_id = kInvalidElementId;
    UUID new_uuid = kInvalidUUID;
    std::unique_ptr<ProcessedElement> processed_element;
    std::unique_ptr<SerializedElement> serialized_element;
  };
  using ElementDataMap = ElementIdHashMap<std::unique_ptr<ElementData>>;

  // This task is responsible performing the mesh-editing geometric operations.
  class CuttingEraserTask : public Task {
   public:
    CuttingEraserTask(std::weak_ptr<SceneGraph> weak_scene_graph,
                      std::weak_ptr<ElementDataMap> weak_data_map,
                      const Mesh& cutting_mesh, GroupId active_group)
        : weak_scene_graph_(weak_scene_graph),
          weak_data_map_(weak_data_map),
          cutting_mesh_(cutting_mesh),
          active_group_(active_group) {}

    // This task requires a PreExecute() phase because the previous task does
    // not return ownership of the ElementData until the OnPostExecute() phase,
    // and they may affect the same element(s).
    bool RequiresPreExecute() const override { return true; }

    // This function determines which elements the cutting stroke might hit, and
    // fetches their meshes and any metadata required to replace them. This task
    // will take ownership of any ElementData that has already been initialized.
    void PreExecute() override;

    // This function performs the mesh split.
    void Execute() override;

    // This returns ownership of the ElementData back to the shared data map.
    void OnPostExecute() override;

   private:
    std::weak_ptr<SceneGraph> weak_scene_graph_;
    std::weak_ptr<ElementDataMap> weak_data_map_;
    ElementDataMap local_data_map_;
    Mesh cutting_mesh_;
    GroupId active_group_;
    std::vector<ElementId> elements_to_cut_;
  };

  // This task is responsible for serializing the mesh split results, and saving
  // them to the SceneGraph.
  class SerializeEraserTask : public Task {
   public:
    // This task takes ownership of the data map -- it's expected that the
    // caller moves, not copies, the shared pointer, and that no other strong
    // references exist.
    SerializeEraserTask(std::weak_ptr<SceneGraph> weak_scene_graph,
                        std::shared_ptr<ElementDataMap> data_map,
                        const settings::Flags& flags, GroupId active_group,
                        const CallbackFlags& callback_flags,
                        IDrawable* drawable_ptr);

    // This task requires a PreExecute() phase because the CuttingEraserTasks
    // not return ownership of the ElementData until the OnPostExecute() phase,
    // which it requires them to commit the result to the SceneGraph.
    bool RequiresPreExecute() const override { return true; }

    // This function fetches IDs for all of the new elements.
    void PreExecute() override;

    // This function fetches the split result and serializes them.
    void Execute() override;

    // This function replaces the element meshes with the results and removes
    // the temporary IDrawable eraser stroke.
    void OnPostExecute() override;

   private:
    std::weak_ptr<SceneGraph> weak_scene_graph_;
    std::shared_ptr<ElementDataMap> data_map_;
    GroupId active_group_;
    UUID active_group_uuid_;
    CallbackFlags callback_flags_;
    std::vector<SceneGraph::ElementAdd> elements_to_add_;
    std::vector<ElementId> elements_to_remove_;
    bool low_memory_mode_;

    // Used only to delete the drawable from the scene graph once the task is
    // complete -- DO NOT DEREFERENCE THIS.
    IDrawable* drawable_ptr_;
  };

  void SetupNewLine(const input::InputData& data, const Camera& camera);
  void Clear();
  void StartCuttingTask(GroupId active_group);
  void StartSerializationTask(GroupId active_group);

  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<input::InputModeler> input_modeler_;
  std::shared_ptr<LineModifierFactory> line_modifier_factory_;
  std::shared_ptr<LayerManager> layer_manager_;
  std::shared_ptr<ITaskRunner> task_runner_;
  std::shared_ptr<settings::Flags> flags_;

  absl::optional<uint32_t> touch_id_;
  MeshRenderer renderer_;
  LineBuilder line_builder_;
  BrushParams::BrushSize size_;

  std::shared_ptr<ElementDataMap> element_data_map_;
  int next_stable_triangle_ = 0;
};

}  // namespace tools
}  // namespace ink

#endif  // INK_ENGINE_REALTIME_STROKE_EDITING_ERASER_H_
