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

#ifndef INK_ENGINE_PUBLIC_HOST_FAKE_SCENE_CHANGE_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_FAKE_SCENE_CHANGE_LISTENER_H_

#include "ink/engine/public/host/iscene_change_listener.h"

namespace ink {

class FakeSceneChangeListener : public ISceneChangeListener {
 public:
  void SceneChanged(
      const proto::scene_change::SceneChangeEvent& scene_change) override {
    num_changes_++;
    changes_.emplace_back(scene_change);
  }
  int NumChanges() { return changes_.size(); }
  proto::scene_change::SceneChangeEvent LatestEvent() {
    return *--changes_.end();
  }
  std::vector<proto::scene_change::SceneChangeEvent> Changes() {
    return changes_;
  }

 private:
  int num_changes_;
  std::vector<proto::scene_change::SceneChangeEvent> changes_;
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_FAKE_SCENE_CHANGE_LISTENER_H_
