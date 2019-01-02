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

#ifndef INK_ENGINE_PUBLIC_HOST_FAKE_ACTIVE_LAYER_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_FAKE_ACTIVE_LAYER_LISTENER_H_

#include "ink/engine/public/host/iactive_layer_listener.h"

namespace ink {

class FakeActiveLayerListener : public IActiveLayerListener {
 public:
  void ActiveLayerChanged(const UUID &uuid,
                          const proto::SourceDetails &sourceDetails) override {
    uuids_.push_back(uuid);
  }

  const std::vector<UUID> &uuids() const { return uuids_; }

 private:
  std::vector<UUID> uuids_;
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_FAKE_ACTIVE_LAYER_LISTENER_H_
