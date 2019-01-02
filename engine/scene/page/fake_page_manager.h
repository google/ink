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

#ifndef INK_ENGINE_SCENE_PAGE_FAKE_PAGE_MANAGER_H_
#define INK_ENGINE_SCENE_PAGE_FAKE_PAGE_MANAGER_H_

#include "ink/engine/scene/page/page_manager.h"

namespace ink {

class FakePageManager : public PageManager {
 public:
  explicit FakePageManager(int num_pages) : num_pages_(num_pages) {}

  int GetNumPages() const override { return num_pages_; }
  bool MultiPageEnabled() const override { return num_pages_ > 0; }

 private:
  int num_pages_;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_PAGE_FAKE_PAGE_MANAGER_H_
