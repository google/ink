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

#ifndef INK_ENGINE_REALTIME_LINE_TOOL_DATA_SINK_H_
#define INK_ENGINE_REALTIME_LINE_TOOL_DATA_SINK_H_

#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/processing/element_converters/line_converter.h"
#include "ink/engine/processing/runner/task_runner.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/data/common/input_points.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/animation/animation_controller.h"

namespace ink {

// LineToolDataSync receives the completed line data from LineTool and spawns
// SceneElementAdder tasks to retesselate the mesh before it is added to the
// scene.
class LineToolDataSink {
 public:
  using SharedDeps = service::Dependencies<SceneGraph, GLResourceManager,
                                           FrameState, ITaskRunner>;

  LineToolDataSink(std::shared_ptr<SceneGraph> graph,
                   std::shared_ptr<GLResourceManager> glr,
                   std::shared_ptr<FrameState> fs,
                   std::shared_ptr<ITaskRunner> task_runner);
  virtual ~LineToolDataSink() {}

  virtual void Accept(const std::vector<FatLine>& lines,
                      std::unique_ptr<InputPoints> input_points,
                      std::unique_ptr<Mesh> rendering_mesh, GroupId group,
                      const ShaderType shader_type,
                      TessellationParams tessellation_params);

 private:
  std::shared_ptr<SceneGraph> scene_graph_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<FrameState> frame_state_;
  std::shared_ptr<ITaskRunner> task_runner_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_LINE_TOOL_DATA_SINK_H_
