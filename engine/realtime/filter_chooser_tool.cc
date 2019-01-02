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

#include "ink/engine/realtime/filter_chooser_tool.h"

#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

static const float kMinDistForDrag = 5.0f;
constexpr DurationS kAnimationDurationSecs(0.05);

using util::Clamp;
using util::Lerp;
using util::Normalize;

FilterChooserTool::FilterChooserTool(const service::UncheckedRegistry& registry)
    : Tool(),
      state_(FilterChooserState::Waiting),
      gl_resources_(registry.GetShared<GLResourceManager>()),
      frame_state_(registry.GetShared<FrameState>()),
      renderer_(gl_resources_),
      current_swipe_progress_(0, registry.GetShared<AnimationController>()) {
  Reset();

  RegisterForInput(registry.GetShared<input::InputDispatch>());
}

FilterChooserTool::~FilterChooserTool() {}

void FilterChooserTool::Update(const Camera& camera, FrameTimeS draw_time) {
  if (!current_swipe_progress_.IsAnimating()) {
    if (state_ == FilterChooserState::Animating) {
      ApplySwipe();
    } else if (state_ == FilterChooserState::AnimatingCancel) {
      Reset();
    }
  }
}

void FilterChooserTool::ApplySwipe() {
  ImageBackgroundState* image_background = nullptr;
  if (gl_resources_->background_state->GetImage(&image_background)) {
    image_background->SetImageFilterEffect(target_filter_);
  } else {
    SLOG(SLOG_ERROR, "No bg to apply filter to");
  }
  Reset();
}

void FilterChooserTool::Reset() {
  state_ = FilterChooserState::Waiting;
  current_swipe_progress_.StopAnimation();
  current_swipe_progress_.SetValue(0);
  still_dragging_expected_direction_ = true;
}

void FilterChooserTool::Enable(bool enabled) {
  // Skip to the end of the animation if we're animating and the tool is being
  // disabled. We don't expect to get any more Input on Draw calls if we're
  // switching away from the tool.
  if (!enabled && state_ != FilterChooserState::Waiting) {
    ApplySwipe();
  }

  Tool::Enable(enabled);
}

void FilterChooserTool::SetProgressAnimationTarget(float target) {
  current_swipe_progress_.AnimateTo(
      target, kAnimationDurationSecs,
      std::unique_ptr<AnimationCurve>(new LinearAnimationCurve()),
      DefaultInterpolator<float>());
}

input::CaptureResult FilterChooserTool::OnInput(const input::InputData& data,
                                                const Camera& camera) {
  if (data.Get(input::Flag::Cancel)) {
    Reset();
    return input::CapResRefuse;
  }

  if (!data.Get(input::Flag::Primary)) {
    return input::CapResCapture;
  }

  switch (state_) {
    case FilterChooserState::Waiting:

      if (data.Get(input::Flag::TDown)) {
        Reset();
        screen_down_x_ = data.screen_pos.x;
        return input::CapResCapture;
      }

      if (std::abs(data.screen_pos.x - screen_down_x_) < kMinDistForDrag) {
        // Do nothing until we detect a drag direction.
        return input::CapResCapture;
      } else {
        // We've started dragging!
        state_ = FilterChooserState::Dragging;

        bool is_right_swipe = data.screen_pos.x > screen_down_x_;

        blit_attrs::FilterEffect current_filter;

        ImageBackgroundState* image_background = nullptr;
        if (gl_resources_->background_state->GetImage(&image_background)) {
          current_filter = image_background->ImageFilterEffect();
        } else {
          SLOG(SLOG_ERROR, "No bg to apply filter to");
          return input::CapResCapture;
        }

        if (is_right_swipe) {
          target_filter_ = NextEffect(current_filter);
        } else {
          target_filter_ = PreviousEffect(current_filter);
        }

        current_swipe_progress_.SetValue(0);
        if (is_right_swipe) {
          // For a right swipe, the left edge stays at left (0), and the right
          // edge slides from left to right (0 to 1).
          swipe_from_ = glm::vec2(0, 0);
          swipe_to_ = glm::vec2(0, 1);

          screen_swipe_from_ = 0;
          screen_swipe_to_ = camera.ScreenDim().x;
        } else {
          // For a left swipe, the right edge stays at right (1), and the left
          // edge slides from right to left (1 to 0).
          swipe_from_ = glm::vec2(1, 1);
          swipe_to_ = glm::vec2(0, 1);

          screen_swipe_from_ = camera.ScreenDim().x;
          screen_swipe_to_ = 0;
        }
      }
      break;
    case FilterChooserState::Animating:
    case FilterChooserState::AnimatingCancel:
      // If we get input while animating, jump straight back into dragging mode.
      state_ = FilterChooserState::Dragging;
      break;
    case FilterChooserState::Dragging:
      break;
  }

  // If we didn't return by now, we should be in a Dragging state.
  ASSERT(state_ == FilterChooserState::Dragging);

  if (data.Get(input::Flag::TUp)) {
    // If the user lifts their finger, either animate the rest of the way to the
    // end or cancel the swipe.
    if (still_dragging_expected_direction_) {
      // Animate to complete the swipe progress.
      state_ = FilterChooserState::Animating;
      SetProgressAnimationTarget(1);
    } else {
      // Animate to cancel the swipe
      state_ = FilterChooserState::AnimatingCancel;
      SetProgressAnimationTarget(0);
    }
  } else {
    // Set a target scroll to wherever the current input is.

    // First figure out how far across the screen the user is.
    float amount_across_screen =
        Normalize(screen_swipe_from_, screen_swipe_to_, data.screen_pos.x);

    // Some input stacks let you go get input positions outside the bounds of
    // the screen (mostly mouse on desktop). Clamp the progress to [0,1].
    float pointer_progress = Clamp(amount_across_screen, 0.0f, 1.0f);

    SetProgressAnimationTarget(pointer_progress);

    still_dragging_expected_direction_ =
        last_pointer_progress_ <= pointer_progress;
    last_pointer_progress_ = pointer_progress;
  }

  return input::CapResCapture;
}

void FilterChooserTool::BeforeSceneDrawn(const Camera& cam,
                                         FrameTimeS draw_time) const {
  // Draw a subset of the background using the target shader.
  ImageBackgroundState* image_background = nullptr;
  if (gl_resources_->background_state->GetImage(&image_background)) {
    Rect bounds = cam.WorldWindow();

    Texture* texture = nullptr;
    if (!gl_resources_->texture_manager->GetTexture(
            image_background->TextureHandle(), &texture)) {
      // Background texture isn't available.
      SLOG(SLOG_ERROR, "Texture unavailable");
      return;
    }

    glm::vec2 left_right_progress =
        Lerp(swipe_from_, swipe_to_, current_swipe_progress_.Value());

    Rect target_rect(bounds);
    target_rect.from.x =
        Lerp(bounds.Left(), bounds.Right(), left_right_progress[0]);
    target_rect.to.x =
        Lerp(bounds.Left(), bounds.Right(), left_right_progress[1]);

    // Don't try to draw empty rects, it makes the renderer sad.
    if (target_rect.Width() <= 0) {
      return;
    }

    renderer_.Draw(
        cam, texture, blit_attrs::Blit(target_filter_), RotRect(target_rect),
        RotRect(image_background->FirstInstanceWorldCoords()).InvertYAxis());
  }
}

void FilterChooserTool::Draw(const Camera& cam, FrameTimeS draw_time) const {}

}  // namespace ink
