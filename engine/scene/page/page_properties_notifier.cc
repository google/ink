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

#include "ink/engine/scene/page/page_properties_notifier.h"

#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

void PagePropertiesNotifier::OnPageBoundsChanged(
    const Rect& bounds, const SourceDetails& source_details) {
  if (source_details.origin != SourceDetails::Origin::Host) {
    proto::Rect r;
    util::WriteToProto(&r, bounds);
    proto::SourceDetails source_details_proto;
    util::WriteToProto(&source_details_proto, source_details);
    page_props_listener_->PageBoundsChanged(r, source_details_proto);
  }
}

}  // namespace ink
