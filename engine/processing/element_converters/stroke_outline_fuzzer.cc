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
#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/processing/element_converters/stroke_outline_converter.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/proto/export_portable_proto.pb.h"

namespace ink {
static void FuzzStrokeOutlineConverter(
    const proto::VectorElements& vector_elements) {
  for (const auto& element : vector_elements.elements()) {
    if (element.has_outline()) {
      ink::StrokeOutlineConverter converter(element.outline());
      static_cast<void>(
          converter.CreateProcessedElement(ink::kInvalidElementId));
    }
  }
}
}  // namespace ink

DEFINE_TEXT_PROTO_FUZZER(const ink::proto::VectorElements& vector_elements) {
  ink::FuzzStrokeOutlineConverter(vector_elements);
}
