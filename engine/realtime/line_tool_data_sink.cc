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

#include "ink/engine/realtime/line_tool_data_sink.h"

#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/processing/element_converters/line_converter.h"
#include "ink/engine/processing/element_converters/scene_element_adder.h"
#include "ink/engine/rendering/scene_drawable.h"

namespace ink {

LineToolDataSink::LineToolDataSink(std::shared_ptr<SceneGraph> graph,
                                   std::shared_ptr<GLResourceManager> glr,
                                   std::shared_ptr<FrameState> fs,
                                   std::shared_ptr<ITaskRunner> task_runner,
                                   std::shared_ptr<settings::Flags> flags)
    : scene_graph_(graph),
      gl_resources_(glr),
      frame_state_(fs),
      task_runner_(task_runner),
      flags_(flags) {}

void LineToolDataSink::Accept(const Camera& down_camera,
                              const std::vector<FatLine>& lines,
                              std::unique_ptr<InputPoints> input_points,
                              std::unique_ptr<Mesh> rendering_mesh,
                              GroupId group, const ShaderType shader_type,
                              TessellationParams tessellation_params) {
  glm::mat4 group_to_world_transform(1);  // identity matrix.
  UUID uuid = scene_graph_->GenerateUUID();

  if (group != kInvalidElementId) {
    // The adder task expects the coordinates to be local to the group.
    ElementMetadata meta = scene_graph_->GetElementMetadata(group);
    // If a group is defined, we should have data for it. Otherwise, things
    // have gone horribly wrong. If this fails, the group transform will
    // be defined as the identity matrix and the line will be in an incorrect
    // place after loading from a save.
    ASSERT(meta.id == group);
    // Save the world transform of the group.
    group_to_world_transform = meta.world_transform;
  }

  std::unique_ptr<LineConverter> converter(new LineConverter(
      lines, group_to_world_transform, down_camera.ScreenToWorld(),
      std::move(input_points), shader_type, tessellation_params));

  std::unique_ptr<SceneElementAdder> adder_task(new SceneElementAdder(
      move(converter), scene_graph_, *flags_, SourceDetails::FromEngine(), uuid,
      kInvalidElementId, group));
  MeshSceneDrawable::AddToScene(scene_graph_->ElementIdFromUUID(uuid), group,
                                *rendering_mesh, scene_graph_, gl_resources_,
                                frame_state_);

  task_runner_->PushTask(move(adder_task));
}

}  // namespace ink
