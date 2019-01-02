// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/engine/rendering/strategy/rendering_strategy.h"

#include "ink/engine/util/dbg/log.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {
namespace util {

template <>
Status ReadFromProto(const ink::proto::RenderingStrategy& proto,
                     RenderingStrategy* rendering_strategy) {
  switch (proto) {
    case ink::proto::BUFFERED_RENDERER:
      *rendering_strategy = RenderingStrategy::kBufferedRenderer;
      return OkStatus();
    case ink::proto::DIRECT_RENDERER:
      *rendering_strategy = RenderingStrategy::kDirectRenderer;
      return OkStatus();
    default:
      *rendering_strategy = RenderingStrategy::kBufferedRenderer;
      return status::InvalidArgument(
          "Unrecognized renderer: $0, using TripleBufferedRenderer.",
          static_cast<int>(proto));
  }
}

}  // namespace util
}  // namespace ink
