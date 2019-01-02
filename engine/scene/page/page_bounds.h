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

#ifndef INK_ENGINE_SCENE_PAGE_PAGE_BOUNDS_H_
#define INK_ENGINE_SCENE_PAGE_PAGE_BOUNDS_H_

#include <memory>

#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/scene/page/page_properties_notifier.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/dependencies.h"

namespace ink {

class PageBounds {
 public:
  using SharedDeps = service::Dependencies<PagePropertiesNotifier>;

  explicit PageBounds(
      std::shared_ptr<PagePropertiesNotifier> page_properties_notifier)
      : page_properties_notifier_(page_properties_notifier) {}

  bool HasBounds() const {
    return bounds_.Area() != 0 || working_bounds_.Area() != 0;
  }

  Rect Bounds() const {
    return working_bounds_.Width() != 0 ? working_bounds_ : bounds_;
  }

  // Returns true if the bounds actually change.
  bool SetBounds(Rect bounds, SourceDetails source) {
    SLOG(SLOG_BOUNDS, "SetBounds($0, $1)", bounds, source);
    if (bounds_ == bounds) return false;
    bounds_ = bounds;
    page_properties_notifier_->OnPageBoundsChanged(bounds_, source);
    return true;
  }

  void SetWorkingBounds(Rect working_bounds) {
    working_bounds_ = working_bounds;
  }

  void ClearWorkingBounds() { working_bounds_ = Rect(); }

  float MinCameraWidth() const { return bounds_.Width() / kZoomInRatio; }

 private:
  std::shared_ptr<PagePropertiesNotifier> page_properties_notifier_;
  Rect bounds_;
  Rect working_bounds_;

  static constexpr float kZoomInRatio = 10.0f;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_PAGE_PAGE_BOUNDS_H_
