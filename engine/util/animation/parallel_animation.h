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

#ifndef INK_ENGINE_UTIL_ANIMATION_PARALLEL_ANIMATION_H_
#define INK_ENGINE_UTIL_ANIMATION_PARALLEL_ANIMATION_H_

#include <memory>
#include <vector>

#include "ink/engine/util/animation/animation.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// An Animation that runs a number of other animations simultaneously.
class ParallelAnimation : public Animation {
 public:
  void Add(std::unique_ptr<Animation> a) { anims_.push_back(std::move(a)); }

  void UpdateImpl(FrameTimeS t) override {
    for (auto& a : anims_) {
      if (!a->HasFinished()) {
        a->Update(t);
      }
    }
  }

  bool HasFinished() const override {
    for (auto& a : anims_) {
      if (!a->HasFinished()) return false;
    }
    return true;
  }

 private:
  std::vector<std::unique_ptr<Animation>> anims_;
};

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ANIMATION_PARALLEL_ANIMATION_H_
