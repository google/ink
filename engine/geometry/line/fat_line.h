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

#ifndef INK_ENGINE_GEOMETRY_LINE_FAT_LINE_H_
#define INK_ENGINE_GEOMETRY_LINE_FAT_LINE_H_

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ink/engine/brushes/size/tip_size_screen.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/line/mid_point.h"
#include "ink/engine/geometry/line/tip/tip_model_factory.h"
#include "ink/engine/geometry/line/tip_type.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// FatLine takes in modeled input data and computes screen coordinates for the
// outlines on either side of the stroke that can be used to create a mesh.
//
// The two sides of the outline (on opposite sides of the stroke) are referred
// to as "backward" and "forward". Most of the actual work of determining the
// outline's vertices is done by the TipModel classes.
class FatLine {
 public:
  typedef std::function<void(glm::vec2 /*centerPt*/, float /*radius*/,
                             InputTimeS /*time*/, float /* pressure */,
                             Vertex* /*vert*/, std::vector<Vertex>* /*line*/)>
      VertAddFn;

  FatLine() : FatLine(30, 20) {}
  FatLine(float radius, uint32_t turn_verts)
      : FatLine(TipSizeScreen({radius, radius}), turn_verts, TipType::Round) {}
  FatLine(TipSizeScreen tip_size, uint32_t turn_verts, TipType tip_type);
  FatLine(const FatLine& other) { *this = other; }
  FatLine& operator=(const FatLine& other);

  // Clears the vertices, leaving an empty line.
  // WARNING: This does not reset the camera, the minimum travel threshold, the
  // tip type, the number of turn vertices, or the vertex-added callback.
  void ClearVertices();

  // Extrude new modeled input point (in screen coordinates). Return true if
  // vertices were added to the fat line as a result.
  // If force is true, extrude point even if distance from the last vertex to
  // the new one doesn't meet min_world_travel_threshold_.
  bool Extrude(glm::vec2 new_pt, InputTimeS time, bool force,
               bool simplify = true);
  void BuildEndCap();

  // Attach this line's start cap to the end vertices of the given line. This
  // FatLine is expected to be empty.
  bool SetStartCapToLineBack(const FatLine& other);

  void SetTurnVerts(uint32_t turn_verts) { turn_verts_ = turn_verts; }

  TipType GetTipType() const {
    EXPECT(tip_model_ != nullptr);
    return tip_model_->GetTipType();
  }
  void SetTipType(TipType tip_type) {
    // tip_model_ should only be nullptr during construction.
    if (tip_model_ == nullptr || tip_model_->GetTipType() != tip_type) {
      tip_model_ = CreateTipModel(tip_type);
    }
  }

  VertAddFn VertCallback() const { return on_add_vert_; }
  void SetVertCallback(VertAddFn function) { on_add_vert_ = function; }

  // New modeled point must move this many pixels before being considered for
  // extrusion.
  float MinScreenTravelThreshold() const {
    return min_screen_travel_threshold_;
  }
  void SetMinScreenTravelThreshold(float distance) {
    min_screen_travel_threshold_ = distance;
  }

  const Camera& DownCamera() const { return down_cam_; }
  void SetDownCamera(const Camera& camera) { down_cam_ = camera; }

  TipSizeScreen TipSize() const { return tip_size_; }
  void SetTipSize(TipSizeScreen tip_size) { tip_size_ = tip_size; }

  void SetStylusState(input::StylusState stylus_state) {
    stylus_state_ = stylus_state;
  }

  float MinRadiusSeen() const { return min_radius_seen_; }
  float MaxRadiusSeen() const { return max_radius_seen_; }

  const std::vector<Vertex>& ForwardLine() const { return fwd_; }
  const std::vector<Vertex>& BackwardLine() const { return back_; }
  const std::vector<Vertex>& StartCap() const { return start_cap_; }
  const std::vector<Vertex>& EndCap() const { return end_cap_; }
  const std::vector<MidPoint>& MidPoints() const { return pts_; }

  std::string ToString() const;

  // Copies the outline of a stroke in screen coordinates to an array of x,y
  // points in object coordinates.  The inverse of the object matrix from an
  // OptimizedMesh must be given.  The outerline of each multiline is used.
  // The lines are copied in the order:
  // lines[0]->start_cap_, lines->fwd_, lines[end]->end_cap_, lines->back_
  static std::vector<glm::vec2> OutlineAsArray(
      const std::vector<FatLine>& lines, const glm::mat4& world_to_object);

 private:
  void BuildStartCap();
  void ExtendLine();
  // Simplify the last n_verts of fwd_ and back_ vertexes to reduce the vertex
  // count. Points are included if they cause the resulting line to shift by at
  // least simplification_threshold.
  // https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
  void Simplify(uint32_t n_verts = 15, float simplification_threshold = 0.1f);

  inline void Append(std::vector<MidPoint>* to, MidPoint p) {
    max_radius_seen_ = std::max(max_radius_seen_, tip_size_.radius);
    min_radius_seen_ = std::min(min_radius_seen_, tip_size_.radius);

    to->push_back(p);
  }

  inline void Append(std::vector<Vertex>* to, glm::vec2 p) {
    max_radius_seen_ = std::max(max_radius_seen_, tip_size_.radius);
    min_radius_seen_ = std::min(min_radius_seen_, tip_size_.radius);

    Vertex v(p);
    if (on_add_vert_)
      on_add_vert_(last_center_, tip_size_.radius, last_extrude_time_,
                   stylus_state_.pressure, &v, to);
    to->push_back(v);
  }

  inline void Append(std::vector<Vertex>* to,
                     const std::vector<glm::vec2>& from) {
    for (auto ai = from.begin(); ai != from.end(); ai++) {
      Append(to, *ai);
    }
  }
  inline void AppendFwd(const std::vector<glm::vec2>& v) { Append(&fwd_, v); }
  inline void AppendBack(const std::vector<glm::vec2>& v) { Append(&back_, v); }
  inline void AppendBack(glm::vec2 p) { Append(&back_, p); }
  inline void AppendFwd(glm::vec2 p) { Append(&fwd_, p); }

  VertAddFn on_add_vert_;

  // extruded pts below this threshold screen distance will be rejected
  float min_screen_travel_threshold_;

  Camera down_cam_;
  TipSizeScreen tip_size_;
  InputTimeS last_extrude_time_;
  float min_radius_seen_;
  float max_radius_seen_;

  input::StylusState stylus_state_;
  std::unique_ptr<TipModel> tip_model_;

  std::vector<Vertex> fwd_;
  std::vector<Vertex> back_;
  std::vector<Vertex> start_cap_;
  std::vector<Vertex> end_cap_;
  std::vector<MidPoint> pts_;

  uint32_t turn_verts_;
  glm::vec2 last_center_{0, 0};

  bool join_to_line_end_;
  MidPoint join_midpoint_;
};

}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_LINE_FAT_LINE_H_
