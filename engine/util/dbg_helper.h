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

#ifndef INK_ENGINE_UTIL_DBG_HELPER_H_
#define INK_ENGINE_UTIL_DBG_HELPER_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/rendering/shaders/mesh_shader.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// DbgHelper is used in several places throughout Ink to draw debugging
// visualizations of various things, e.g. mesh geometry, input points,
// predictions, blit rects, etc.
//
// To use the DbgHelper, enable it in the service registry prior to constructing
// the engine in place of the default NoopDbgHelper, e.g.
//
//    auto registry = extensions::GetServiceDefinitions();
//    registry->DefineService<IDbgHelper, DbgHelper>();
//
// Then enable whichever debug visualizations of interest in the relevant
// classes, e.g. LineTool, DbgInputVisualizer, Native Client module, etc.
// Run your favorite Ink app and then see the pretty colors!
//
// This can also be used in golden image tests by enabling the DbgHelper in the
// SEngineTestEnv.
class IDbgHelper {
 public:
  IDbgHelper() {}
  virtual ~IDbgHelper() {}

  virtual void Draw(const Camera& cam, FrameTimeS draw_time) = 0;

  // ID is used to identify groups of points, rects, lines, and meshes for later
  // removal by ID.  IDs should be unique *per visualization*, the shapes are
  // stored in a multimap by ID.  This can be used to reset a visualization,
  // e.g. between lines or frames.

  virtual void AddPoint(Vertex vert, float size, uint32_t id) = 0;

  virtual void AddRect(Rect r, glm::vec4 color, bool fill, uint32_t id) = 0;

  virtual void AddLine(Vertex from_world, Vertex to_world, float size_screen,
                       uint32_t id) = 0;

  virtual void AddMeshSkeleton(const Mesh& m, glm::vec4 edge_color,
                               glm::vec4 point_color, uint32_t id) {}

  virtual void AddMesh(const Mesh& m, uint32_t id) = 0;
  virtual void AddMesh(const Mesh& m, glm::vec4 color, uint32_t id) = 0;

  virtual void Remove(uint32_t id) = 0;
  virtual void Clear() = 0;

  // Enable the various visualizations
  virtual bool PredictedLineVisualizationEnabled() = 0;
  virtual void EnablePredictedLineVisualization(bool enable) = 0;
};

class NoopDbgHelper : public IDbgHelper {
 public:
  NoopDbgHelper() {}

  void Draw(const Camera& cam, FrameTimeS draw_time) override {}

  void AddPoint(Vertex vert, float size, uint32_t id) override {}

  void AddRect(Rect r, glm::vec4 color, bool fill, uint32_t id) override {}

  void AddLine(Vertex from_world, Vertex to_world, float size_screen,
               uint32_t id) override {}

  void AddMeshSkeleton(const Mesh& m, glm::vec4 edge_color,
                       glm::vec4 point_color, uint32_t id) override {}

  void AddMesh(const Mesh& m, uint32_t id) override {}
  void AddMesh(const Mesh& m, glm::vec4 color, uint32_t id) override {}

  void Remove(uint32_t id) override {}
  void Clear() override {}

  bool PredictedLineVisualizationEnabled() override { return false; }
  void EnablePredictedLineVisualization(bool enable) override {}
};

class DbgHelper : public IDbgHelper {
 public:
  using SharedDeps = service::Dependencies<GLResourceManager>;

  explicit DbgHelper(const service::UncheckedRegistry& registry);
  explicit DbgHelper(const std::shared_ptr<GLResourceManager>& gl_resources);

  void Draw(const Camera& cam, FrameTimeS draw_time) override;

  void AddPoint(Vertex vert, float size, uint32_t id) override;

  void AddRect(Rect r, glm::vec4 color, bool fill, uint32_t id) override;

  void AddLine(Vertex from_world, Vertex to_world, float size_screen,
               uint32_t id) override;

  void AddMeshSkeleton(const Mesh& m, glm::vec4 edge_color,
                       glm::vec4 point_color, uint32_t id) override;

  void AddMesh(const Mesh& m, uint32_t id) override;
  void AddMesh(const Mesh& m, glm::vec4 color, uint32_t id) override;

  void Remove(uint32_t id) override;
  void Clear() override;

  bool PredictedLineVisualizationEnabled() override {
    return predicted_line_visualization_enabled_;
  }
  void EnablePredictedLineVisualization(bool enable) override {
    predicted_line_visualization_enabled_ = enable;
  }

 private:
  void DrawPoint(const Camera& cam, FrameTimeS draw_time, glm::vec2 position,
                 glm::vec4 color, float size);
  void DrawLine(const Camera& cam, FrameTimeS draw_time, glm::vec2 start,
                glm::vec2 end, glm::vec4 color, float size_screen);
  void DrawRect(const Camera& cam, FrameTimeS draw_time, const Rect& r,
                const glm::vec4& color, bool fill);
  void DrawSkeleton(const Camera& cam, FrameTimeS draw_time, const Mesh& mesh,
                    glm::vec4 edge_color, glm::vec4 point_color);
  void DrawMesh(const Camera& cam, FrameTimeS draw_time, Mesh* mesh);

  struct DbgPoint {
    glm::vec2 position{0, 0};
    glm::vec4 color{0, 0, 0, 0};
    float size = 0;
  };

  struct DbgLine {
    glm::vec2 start{0, 0};
    glm::vec2 end{0, 0};
    glm::vec4 color{0, 0, 0, 0};
    float size_screen = 0;
  };

  struct DbgRect {
    Rect r;
    glm::vec4 color{0, 0, 0, 0};
    bool fill = true;
  };

  struct DbgMeshSkeleton {
    Mesh mesh;
    glm::vec4 edge_color{0, 0, 0, 0};
    glm::vec4 point_color{0, 0, 0, 0};
  };

  std::unordered_multimap<uint32_t, DbgPoint> points_;
  std::unordered_multimap<uint32_t, DbgLine> lines_;
  std::unordered_multimap<uint32_t, DbgRect> rects_;
  std::unordered_multimap<uint32_t, DbgMeshSkeleton> skeletons_;
  std::unordered_multimap<uint32_t, Mesh> meshes_;

  std::shared_ptr<GLResourceManager> gl_resources_;
  MeshRenderer renderer_;

  bool predicted_line_visualization_enabled_ = false;
};

}  // namespace ink
#endif  // INK_ENGINE_UTIL_DBG_HELPER_H_
