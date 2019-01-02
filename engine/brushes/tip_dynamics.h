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

#ifndef INK_ENGINE_BRUSHES_TIP_DYNAMICS_H_
#define INK_ENGINE_BRUSHES_TIP_DYNAMICS_H_

#include "ink/engine/brushes/brushes.h"
#include "ink/engine/brushes/size/tip_size_world.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/input/stylus_state_modeler.h"
#include "ink/engine/util/funcs/piecewise_interpolator.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// If true, the raw input locations will be used instead of modeled input.
// This includes the position smoothing in TipDynamics and the modeled input
// generation in PhysicsInputModeler.
static const bool kDebugRawInput = false;

// Given a sequence of input points, performs smoothing operations and
// dynamically changes stroke width based on a physics model.  Through
// "TipShapeParams" supports configuration for how stroke width is generated.
//
// See README.md for a mathematical description of the update equations.
class TipDynamics {
 public:
  struct TipState {
    glm::vec2 world_position{0, 0};
    TipSizeWorld tip_size;
    input::StylusState stylus_state;

    TipState(glm::vec2 world_position_in, TipSizeWorld tip_size_in,
             input::StylusState stylus_state_in)
        : world_position(world_position_in),
          tip_size(tip_size_in),
          stylus_state(stylus_state_in) {}
  };

  struct ModelConstants {
    // Velocity is multiplicatively dampened by  (1-"shapeDrag") at each tick.
    // Should be in range (0,1).  See README.md.
    float shape_drag;

    // More mass results in smaller moves in the position with each tick.
    // Should be > 0.  See README.md.
    float shape_mass;

    // If true, position output is equal to input. Does not affect input
    // sampling.
    bool no_position_modeling = false;
  };

 public:
  TipDynamics();
  explicit TipDynamics(const ModelConstants& model_constants);

  // Large values result in wider lines generated.  "s" must be in the interval
  // [0,1].
  void SetSize(float s) { size_ = s; }

  void SetModelConstants(ModelConstants mc) { model_constants_ = mc; }
  ModelConstants GetModelConstants() const { return model_constants_; }

  // Clears the state of the internal physics model to begin a new stroke with
  // the given input.
  void Reset(const input::InputData& input);

  // Given an input point, updates the physics model and produces a modeled
  // input point.
  TipState Tick(glm::vec2 new_position_world, InputTimeS time,
                const Camera& cam);

  // Inform tip dynamics of the given raw input (such that it can be used for
  // pressure computation). Must be called after Reset(), lest values will be
  // lost.
  void AddRawInputData(const input::InputData& input);

  void ModSpeedForStrokeEnd(float multiplier);

  glm::vec2 VelocityWorld() const { return velocity_; }

 private:
  void UpdateSpeed(InputTimeS time, double world_dist,
                   glm::vec2 new_position_world, Camera cam);
  TipSizeWorld GetTipSize(input::StylusState stylus_state, float screen_dist,
                          const Camera& cam);

 public:
  // Configurable variables used to compute the variable width component of a
  // stroke.
  BrushParams::TipShapeParams params_;

 private:
  ModelConstants model_constants_;
  input::StylusStateModeler stylus_modeler_;
  float size_;

  InputTimeS time_;

  // Speed value used to compute radius (when appropriate). Unlike velocity_,
  // this value reflects things like speed penalties for direction changes.
  // cm/s
  float speed_;

  TipSizeWorld tip_size_;
  glm::vec2 position_{0, 0};  // World coordinates
  glm::vec2 velocity_{0, 0};  // World coordinates
  float previous_pressure_;
};

}  // namespace ink
#endif  // INK_ENGINE_BRUSHES_TIP_DYNAMICS_H_
