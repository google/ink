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

// Translates page property-related changes from internal engine terms into
// protos that an IPagePropertiesListener can understand.

#ifndef INK_ENGINE_SCENE_PAGE_PAGE_PROPERTIES_NOTIFIER_H_
#define INK_ENGINE_SCENE_PAGE_PAGE_PROPERTIES_NOTIFIER_H_

#include <memory>

#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/host/ipage_properties_listener.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/service/dependencies.h"

namespace ink {

class PagePropertiesNotifier {
 public:
  using SharedDeps = service::Dependencies<IPagePropertiesListener>;

  explicit PagePropertiesNotifier(
      std::shared_ptr<IPagePropertiesListener> page_props_listener)
      : page_props_listener_(page_props_listener) {}
  virtual ~PagePropertiesNotifier() {}

  virtual void OnPageBoundsChanged(const Rect& bounds,
                                   const SourceDetails& source_details);

 private:
  std::shared_ptr<IPagePropertiesListener> page_props_listener_;
};

// For testing.
class FakePagePropertiesNotifier : public PagePropertiesNotifier {
 public:
  FakePagePropertiesNotifier() : PagePropertiesNotifier(nullptr) {}
  void OnPageBoundsChanged(const Rect& bounds,
                           const SourceDetails& source_details) override {}
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_PAGE_PAGE_PROPERTIES_NOTIFIER_H_
