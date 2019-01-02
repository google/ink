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

#ifndef INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_BUNDLE_PROTO_CONVERTER_H_
#define INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_BUNDLE_PROTO_CONVERTER_H_

#include <memory>

#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

class BundleProtoConverter : public IElementConverter {
 public:
  explicit BundleProtoConverter(const proto::ElementBundle& unsafe_bundle);

  // Disallow copy and assign.
  BundleProtoConverter(const BundleProtoConverter&) = delete;
  BundleProtoConverter& operator=(const BundleProtoConverter&) = delete;

  std::unique_ptr<ProcessedElement> CreateProcessedElement(
      ElementId id) override;

 private:
  const proto::ElementBundle unsafe_proto_bundle_;
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_BUNDLE_PROTO_CONVERTER_H_
