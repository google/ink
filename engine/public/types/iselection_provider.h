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

#ifndef INK_ENGINE_PUBLIC_TYPES_ISELECTION_PROVIDER_H_
#define INK_ENGINE_PUBLIC_TYPES_ISELECTION_PROVIDER_H_

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/scene/page/page_manager.h"

namespace ink {
class ISelectionProvider {
 public:
  virtual ~ISelectionProvider() {}

  // Given two world coordinate points that represent a selection gesture,
  // GetSelection populates the out vector with the deduced user selection. If
  // this function succeeds, it returns OkStatus(), and populates the given out
  // param with selected rectangles. If it fails for whatever reason, it returns
  // a descriptive error status, and clears the given out param.
  virtual S_WARN_UNUSED_RESULT Status GetSelection(
      glm::vec2 start_world, glm::vec2 end_world,
      const PageManager& page_manager, std::vector<Rect>* out) const = 0;

  // Given a world coordinate point, determines whether the given point is "in"
  // some text, i.e., is close enough to a glyph to be reasonably interpreted as
  // in the glyph's hit box for selection.
  virtual S_WARN_UNUSED_RESULT bool IsInText(
      glm::vec2 world, const PageManager& page_manager) const = 0;

  // This function is for debugging text selections.
  virtual std::vector<Rect> GetCandidateRects(
      glm::vec2 world, const PageManager& page_manager) const = 0;
};
}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_TYPES_ISELECTION_PROVIDER_H_
