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

#ifndef INK_ENGINE_REALTIME_PAN_ZOOM_CONSTRAINT_H_
#define INK_ENGINE_REALTIME_PAN_ZOOM_CONSTRAINT_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/drag_reco.h"
#include "ink/engine/scene/page/page_bounds.h"

namespace ink {

// RequiredRectDragModifier takes in input drag events and modifies them as
// needed in order to ensure that a given screen-coordinates Rect is always
// within the page bounds. Also ensure that any scaling by touch honors a given
// minimum scale ratio.
class RequiredRectDragModifier {
 public:
  RequiredRectDragModifier(std::shared_ptr<Camera> camera,
                           std::shared_ptr<PageBounds> page_bounds);
  virtual ~RequiredRectDragModifier() {}

  // The required Rect, in screen coordinates, must be kept within page bounds.
  // Scaling by touch cannot allow the camera to go below the given scale.
  // A value of 0 allows arbitrary scaling, a value of 1 requires that the
  // document is 1:1 with the screen or larger.
  void StartEnforcement(const Rect& Rect, float minimum_scale);

  // Stop enforcing required box and scale.
  void StopEnforcement();

  // If needed, modify the given drag event to enforce the required Rect
  // invariant.
  void ConstrainDragEvent(input::DragData* drag);

  // RequiredRectDragModifier is neither copyable nor movable.
  RequiredRectDragModifier(const RequiredRectDragModifier&) = delete;
  RequiredRectDragModifier& operator=(const RequiredRectDragModifier&) = delete;

 private:
  // Returns true if the page bounds contain the current required_rect_ for the
  // given Camera.
  bool PageBoundsContainsRequired(const Camera& camera);

  bool enabled_;
  float minimum_scale_;
  Rect required_rect_;
  std::shared_ptr<Camera> camera_;
  std::shared_ptr<PageBounds> page_bounds_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_PAN_ZOOM_CONSTRAINT_H_
