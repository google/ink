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

#include "ink/engine/scene/graph/element_notifier.h"

#include <cstddef>

#include <utility>

#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/scene/types/element_metadata.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/helpers.h"

namespace ink {

using ink::util::WriteToProto;

ElementNotifier::ElementNotifier(
    std::shared_ptr<IElementListener> element_listener)
    : element_listener_(element_listener) {
  ASSERT(element_listener);
  source_to_callback_flags_[SourceDetails::FromEngine()] =
      CallbackFlags::IdAndFullStroke();
  source_to_callback_flags_[SourceDetails::FromHost(0)] =
      CallbackFlags::NoCallback();

  source_to_callback_flags_[SourceDetails::FromHost(1)] =
      CallbackFlags::IdAndFullStroke();

  source_to_callback_flags_[SourceDetails::EngineInternal()] =
      CallbackFlags::NoCallback();
}

void ElementNotifier::SetCallbackFlags(const SourceDetails& source_details,
                                       const CallbackFlags& callback_flags) {
  source_to_callback_flags_[source_details] = callback_flags;
}

CallbackFlags ElementNotifier::GetCallbackFlags(
    const SourceDetails& source_details) const {
  if (source_to_callback_flags_.count(source_details)) {
    return source_to_callback_flags_.at(source_details);
  } else {
    SLOG(SLOG_ERROR, "No callback mappings for source $0", source_details);
    return CallbackFlags::NoCallback();
  }
}

void ElementNotifier::OnElementsAdded(
    const std::vector<SerializedElement>& serialized_elements, UUID below_uuid,
    const SourceDetails& source) {
  ink::proto::SourceDetails source_details;
  ink::util::WriteToProto(&source_details, source);

  proto::ElementBundleAdds adds;
  for (const auto& serialized_element : serialized_elements) {
    auto flags = serialized_element.callback_flags;
    if (flags.do_callback) {
      // The bundle on line already conditionally has CTM data at this point,
      // and will always have UUID set.
      ProtoHelpers::AddElementBundleAdd(*serialized_element.bundle, below_uuid,
                                        &adds);
    }
  }

  if (adds.element_bundle_add_size() != 0) {
    element_listener_->ElementsAdded(adds, source_details);
  }
}

void ElementNotifier::OnElementsRemoved(const std::vector<UUID>& uuids,
                                        const SourceDetails& source) {
  auto cflags = GetCallbackFlags(source);
  if (cflags.do_callback) {
    ink::proto::SourceDetails source_details;
    ink::util::WriteToProto(&source_details, source);
    ink::proto::ElementIdList proto_ids;
    for (const auto& uuid : uuids) {
      proto_ids.add_uuid(uuid);
    }
    element_listener_->ElementsRemoved(proto_ids, source_details);
  }
}

void ElementNotifier::OnElementsReplaced(
    const std::vector<UUID>& elements_to_remove,
    const std::vector<SerializedElement>& elements_to_add,
    const std::vector<UUID>& elements_to_add_below,
    const SourceDetails& source_details) {
  ASSERT(elements_to_add.size() == elements_to_add_below.size());

  auto callback_flags = GetCallbackFlags(source_details);
  if (!callback_flags.do_callback) return;

  proto::ElementBundleReplace replace_proto;
  for (const auto& uuid : elements_to_remove)
    replace_proto.mutable_elements_to_remove()->add_uuid(uuid);
  for (int i = 0; i < elements_to_add.size(); ++i)
    ProtoHelpers::AddElementBundleAdd(*elements_to_add[i].bundle,
                                      elements_to_add_below[i],
                                      replace_proto.mutable_elements_to_add());

  if (replace_proto.elements_to_add().element_bundle_add_size() > 0 ||
      replace_proto.elements_to_remove().uuid_size() > 0) {
    proto::SourceDetails source_details_proto;
    util::WriteToProto(&source_details_proto, source_details);
    element_listener_->ElementsReplaced(replace_proto, source_details_proto);
  }
}

void ElementNotifier::OnElementsMutated(
    const std::vector<ElementMutationData>& mutation_data,
    const SourceDetails& source) {
  auto cflags = GetCallbackFlags(source);
  if (!cflags.do_callback) {
    return;
  }

  ink::proto::ElementTransformMutations mutations;
  for (const auto& data : mutation_data) {

    if (data.mutation_type == ElementMutationType::kTransformMutation) {
      ink::proto::AffineTransform tx;
      // We only persist the group transforms. World transforms are recomputed
      // via the group's object to world transform and element's object to group
      // transform.
      WriteToProto(&tx, data.modified_element_data.group_transform);
      AppendElementTransform(data.modified_element_data.uuid, tx, &mutations);
    }
  }

  if (mutations.mutation_size() > 0) {
    ink::proto::SourceDetails source_details;
    ink::util::WriteToProto(&source_details, source);
    element_listener_->ElementsTransformMutated(mutations, source_details);
  }
}
}  // namespace ink
