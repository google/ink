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

#include <string>
#include "security/fuzzing/blaze/proto_message_mutator.h"
#include "ink/engine/processing/element_converters/bundle_proto_converter.h"
#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
static void FuzzBundleProtoConverter(const proto::Snapshot& snapshot) {
  for (const auto& element : snapshot.element()) {
    SLOG(SLOG_INFO, "Element: $0", element.DebugString());
    ink::BundleProtoConverter converter(element);
    static_cast<void>(converter.CreateProcessedElement(ink::kInvalidElementId));
  }
}
}  // namespace ink

DEFINE_TEXT_PROTO_FUZZER(const ink::proto::Snapshot& snapshot) {
  ink::FuzzBundleProtoConverter(snapshot);
}
