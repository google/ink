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

#ifndef INK_ENGINE_GEOMETRY_LINE_TIP_CHISEL_TIP_MODEL_H_
#define INK_ENGINE_GEOMETRY_LINE_TIP_CHISEL_TIP_MODEL_H_

#include "ink/engine/geometry/line/mid_point.h"
#include "ink/engine/geometry/line/tip/tip_model.h"
#include "ink/engine/geometry/line/tip/tip_utils.h"
#include "ink/engine/geometry/primitives/segment.h"

namespace ink {

// ChiselTipModel models points as a rectangle with a circle on each end.
//
// Each input MidPoint defines the location of these two circles. The head
// circle is centered at the MidPoint center (the position of the touch event).
// The tail circle is centered 2 * radius away along the stylus orientation.
//
// When the stroke reverses directions, it's possible that both of these points
// need to be put on the same side of the FatLine. This means that the previous
// or next point to connect to on the same side isn't always knowable from a
// 3-MidPoint window. Thus, we accumulate points to add to each side and keep
// our own window to ensure we're connecting the correct circles.
class ChiselTipModel : public TipModel {
 public:
  ChiselTipModel() : left_(true), right_(false) { Clear(); }

  void AddTurnPoints(const MidPoint& start, const MidPoint& middle,
                     const MidPoint& end, int turn_vertices,
                     std::function<void(glm::vec2 pt)> add_left,
                     std::function<void(glm::vec2 pt)> add_right) override;

  TipType GetTipType() const override { return TipType::Chisel; }

  void Clear() override;
 private:
  struct SidePoint {
    glm::vec2 position{0, 0};
    float radius = -1;

    SidePoint() = default;
    SidePoint(glm::vec2 position_in, float radius_in)
        : position(position_in), radius(radius_in) {}
  };
  // Side stores the last two SidePoints for a side of the line.
  class Side {
   public:
    // Negative radius means undefined point.
    explicit Side(bool is_left) : is_left_(is_left) {}
    // Add the given vertex to the side. Radius must be >= 0.
    void Push(SidePoint point);
    // True if the current and previous points have values.
    bool IsPopulated() const { return previous_.radius >= 0; }
    void Clear();

    SidePoint Current() const { return current_; }
    SidePoint Previous() const { return previous_; }
    bool IsLeft() const { return is_left_; }
    // When a turn happens and both points went to the other side, we need the
    // points to potentially create an intersection point once the next point
    // on this side comes in. These are only used once each time it's set.
    void SetTurnPoints(SidePoint first, SidePoint second);
    void ClearTurn();
    SidePoint FirstTurn() const { return first_turn_; }
    SidePoint SecondTurn() const { return second_turn_; }

   private:
    SidePoint current_;
    SidePoint previous_;
    SidePoint first_turn_;
    SidePoint second_turn_;
    bool is_left_;
  };

  Side left_;
  Side right_;

  void AddToSide(glm::vec2 v, float radius, int turn_vertices, Side* side,
                 std::function<void(glm::vec2 pt)> add);
  void AddTangents(SidePoint start, SidePoint middle, SidePoint end,
                   bool is_left, int turn_vertices,
                   std::function<void(glm::vec2 pt)> add);
  // Returns true if the 'first' vertex was inserted first, false if 'second'
  // went first.
  bool AddBothToSide(glm::vec2 first, glm::vec2 second, float radius,
                     int turn_vertices, Side* side,
                     std::function<void(glm::vec2 pt)> add);
  static Segment FindLineTangent(glm::vec2 start, float start_radius,
                                 glm::vec2 end, float end_radius, bool left);
  static glm::vec2 GetTailCenter(const MidPoint& point);
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_LINE_TIP_CHISEL_TIP_MODEL_H_
