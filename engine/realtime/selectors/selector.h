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

#ifndef INK_ENGINE_REALTIME_SELECTORS_SELECTOR_H_
#define INK_ENGINE_REALTIME_SELECTORS_SELECTOR_H_

#include <memory>
#include <utility>
#include <vector>

#include "ink/engine/camera/camera.h"
#include "ink/engine/input/input_data.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/scene/types/drawable.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

// Abstract base class for making spatial queries of elements in the scene,
// based on input.
class Selector : public IDrawable {
 public:
  Selector() {}

  virtual input::CaptureResult OnInput(const input::InputData &data,
                                       const Camera &camera) = 0;
  virtual bool HasSelectedElements() const = 0;
  virtual std::vector<ElementId> SelectedElements() const = 0;
  virtual void Reset() = 0;

  std::function<bool(const ElementId &)> Filter() { return filter_; }
  void SetFilter(std::function<bool(const ElementId &)> filter) {
    filter_ = std::move(filter);
  }

 private:
  std::function<bool(const ElementId &)> filter_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_SELECTORS_SELECTOR_H_
