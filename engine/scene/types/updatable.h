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

#ifndef INK_ENGINE_SCENE_TYPES_UPDATABLE_H_
#define INK_ENGINE_SCENE_TYPES_UPDATABLE_H_

#include "ink/engine/camera/camera.h"
#include "ink/engine/scene/types/event_dispatch.h"

namespace ink {

class IUpdatable {
 public:
  virtual ~IUpdatable() {}
  virtual void Update(const Camera& cam) = 0;
};

class UpdateListener : public IUpdatable,
                       public EventListener<UpdateListener> {};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_TYPES_UPDATABLE_H_
