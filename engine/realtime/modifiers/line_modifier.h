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

#ifndef INK_ENGINE_REALTIME_MODIFIERS_LINE_MODIFIER_H_
#define INK_ENGINE_REALTIME_MODIFIERS_LINE_MODIFIER_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/brushes.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/util/signal_filters/exp_moving_avg.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {
class ILineAnimation;
class LineModifierFactory;

struct LineModParams {
  // (hint) How many points to extrude before segmenting the line
  int split_n;
  // (hint) Min/max number of verts for turn extrusion. Actual number
  // of verts is a result of interpolating across this range based on
  // the screen space size of the line
  glm::ivec2 n_verts{0, 0};
  // Whether to run realtime constrained delauny refinement
  bool refine_mesh;
  // Whether there's a color linearization step between neighbors
  bool linearize_mesh_verts;
  // Whether there's a color linearization step between neighbors for
  // verticies created via tessellation (and weren't piped through the
  // modifier)
  bool linearize_combined_verts;
  // URI of the texture to use for drawing this line (empty if this isn't a
  // textured line).
  string texture_uri;

  // Given a screen radius, returns a hint for how many verts should
  // be used to make a turn. Based on nVerts
  int NVertsAtRadius(float radius_screen) const;

  LineModParams();

  bool operator==(const LineModParams& other) const {
    return split_n == other.split_n && n_verts == other.n_verts &&
           refine_mesh == other.refine_mesh &&
           linearize_mesh_verts == other.linearize_mesh_verts &&
           linearize_combined_verts == other.linearize_combined_verts &&
           texture_uri == other.texture_uri;
  }
};

class LineModifier {
 public:
  LineModifier(const LineModParams& params, const glm::vec4& rgb);
  virtual ~LineModifier();

  virtual void SetupNewLine(const input::InputData& data, const Camera& camera);
  virtual void OnAddVert(Vertex* vert, glm::vec2 center_pt, float radius,
                         const std::vector<Vertex>& line, float pressure);
  virtual void Tick(float screen_radius, glm::vec2 new_position_screen,
                    InputTimeS time, const Camera& cam);

  // Called before the line is committed to the scenegraph.
  //
  // Returns true if lineSegments is modified. (returning true has best case
  // nlog(n) perf cost, where n=num verts)
  virtual bool ModifyFinalLine(std::vector<FatLine>* line_segments);

  virtual void ApplyAnimationToVert(Vertex* vert, glm::vec2 center_pt,
                                    float radius, DurationS time_since_tdown,
                                    const std::vector<Vertex>& line);
  virtual ShaderType GetShaderType();

  virtual float GetMinScreenTravelThreshold(const Camera& cam);

  const LineModParams& params() const { return params_; }
  LineModParams& mutable_params() { return params_; }

 protected:
  BrushParams brush_params_;
  LineModParams params_;
  glm::vec4 rgba_{0, 0, 0, 0};

  float speed_;
  glm::vec2 last_pos_{0, 0};
  float distance_traveled_cm_;
  InputTimeS last_time_;
  Camera cam_;
  signal_filters::ExpMovingAvg<double, double> noise_filter_;
  std::unique_ptr<ILineAnimation> animation_;

  friend class LineModifierFactory;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_MODIFIERS_LINE_MODIFIER_H_
