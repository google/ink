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

#include "ink/engine/processing/element_converters/bundle_proto_converter.h"

#include "ink/engine/processing/element_converters/bezier_path_converter.h"
#include "ink/engine/scene/data/common/mesh_serializer_provider.h"
#include "ink/engine/scene/data/common/stroke.h"
#include "ink/engine/scene/types/element_attributes.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

using std::unique_ptr;

BundleProtoConverter::BundleProtoConverter(
    const ink::proto::ElementBundle& unsafe_bundle)
    : unsafe_proto_bundle_(unsafe_bundle) {}

unique_ptr<ProcessedElement> BundleProtoConverter::CreateProcessedElement(
    ElementId id, const ElementConverterOptions& options) {
  ink::ElementBundle bundle;
  if (ink::util::ReadFromProto(unsafe_proto_bundle_, &bundle)) {
    ink::proto::Element element = bundle.unsafe_element();

    // Stroke.
    if (element.has_stroke()) {
      ink::Stroke stroke;
      if (ink::util::ReadFromProto(bundle, &stroke)) {
        ElementAttributes attributes;
        if (!ink::util::ReadFromProto(bundle.unsafe_element().attributes(),
                                      &attributes)) {
          SLOG(SLOG_ERROR, "Unable to read attributes from proto.");
        }
        std::unique_ptr<ProcessedElement> processed_element;
        if (ProcessedElement::Create(id, stroke, attributes,
                                     options.low_memory_mode,
                                     &processed_element)) {
          return processed_element;
        }
      }
    }

    // Path.
    if (element.has_path()) {
      const auto& path = element.path();
      BezierPathConverter converter(path);
      unique_ptr<ProcessedElement> line =
          converter.CreateProcessedElement(id, options);
      if (!line) {
        SLOG(SLOG_ERROR, "Failed to create processed line from path");
        return nullptr;
      }
      return line;
    }
  }
  SLOG(SLOG_ERROR, "Failed to deserialize bundle");
  return nullptr;
}
}  // namespace ink
