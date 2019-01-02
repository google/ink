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

#ifndef INK_ENGINE_REALTIME_FILTER_CHOOSER_TOOL_H_
#define INK_ENGINE_REALTIME_FILTER_CHOOSER_TOOL_H_

#include <memory>
#include <string>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/baseGL/textured_quad_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/animation/animated_value.h"
#include "ink/engine/util/animation/animation_controller.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// FilterChooserTool allows the user to swipe horizontally in order to cycle
// through a set of predefined background filters (see
// blit_attrs::FilterEffect).
class FilterChooserTool : public Tool {
 public:
  explicit FilterChooserTool(const service::UncheckedRegistry& registry);
  ~FilterChooserTool() override;

  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& live_camera) override;

  void Update(const Camera& cam, FrameTimeS draw_time) override;

  void BeforeSceneDrawn(const Camera& cam, FrameTimeS draw_time) const override;

  void Draw(const Camera& cam, FrameTimeS draw_time) const override;

  void Enable(bool enabled) override;

  inline std::string ToString() const override { return "<FilterChooserTool>"; }

 private:
  void Reset();
  void SwipeAnimationUpdate(float value);
  void ApplySwipe();
  void SetProgressAnimationTarget(float target);

  enum class FilterChooserState {
    Waiting,
    Dragging,         // We are animating with the target of the user's finger.
    Animating,        // User's finger is up, animating to apply the swipe.
    AnimatingCancel,  // User's finger is up, animating to cancel
  } state_;
  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<FrameState> frame_state_;

  TexturedQuadRenderer renderer_;

  blit_attrs::FilterEffect target_filter_ = blit_attrs::FilterEffect::None;

  float screen_down_x_ = 0;

  glm::vec2 swipe_from_{0, 0};  // Left and right sides of the swipe from.
  glm::vec2 swipe_to_{0, 0};    // Left and right sides of the swipe to.

  // The from and to of the swipe in screen space, used to determine how much
  // "progress" in the swipe is represented by the user's finger at a particular
  // coordinate.
  float screen_swipe_from_;
  float screen_swipe_to_;

  // 0 from the initial state, 1 from the fully-swiped state.
  AnimatedValue<float> current_swipe_progress_;

  bool still_dragging_expected_direction_ = true;
  float last_pointer_progress_ = 0;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_FILTER_CHOOSER_TOOL_H_
