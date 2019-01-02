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

#ifndef INK_ENGINE_RENDERING_STRATEGY_RENDERING_STRATEGY_H_
#define INK_ENGINE_RENDERING_STRATEGY_RENDERING_STRATEGY_H_

#include "ink/engine/public/types/status.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

enum class RenderingStrategy {
  kBufferedRenderer,
  kDirectRenderer,
};

namespace util {

template <>
ABSL_MUST_USE_RESULT Status
ReadFromProto(const proto::RenderingStrategy& proto,
              RenderingStrategy* rendering_strategy);

}  // namespace util
}  // namespace ink

#endif  // INK_ENGINE_RENDERING_STRATEGY_RENDERING_STRATEGY_H_
