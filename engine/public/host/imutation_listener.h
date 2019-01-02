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

#ifndef INK_ENGINE_PUBLIC_HOST_IMUTATION_LISTENER_H_
#define INK_ENGINE_PUBLIC_HOST_IMUTATION_LISTENER_H_

#include "ink/engine/scene/types/event_dispatch.h"
#include "ink/proto/mutations_portable_proto.pb.h"

namespace ink {

class IMutationListener : public EventListener<IMutationListener> {
 public:
  ~IMutationListener() override {}

  virtual void OnMutation(
      const proto::mutations::Mutation& unsafe_mutation) = 0;
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_HOST_IMUTATION_LISTENER_H_
