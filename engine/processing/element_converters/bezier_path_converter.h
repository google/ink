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

#ifndef INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_BEZIER_PATH_CONVERTER_H_
#define INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_BEZIER_PATH_CONVERTER_H_

#include <memory>

#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

// Converts the Path proto to a processed line.
class BezierPathConverter : public IElementConverter {
 public:
  explicit BezierPathConverter(const proto::Path& unsafe_path);

  // Disallow copy and assign.
  BezierPathConverter(const BezierPathConverter&) = delete;
  BezierPathConverter& operator=(const BezierPathConverter&) = delete;

  std::unique_ptr<ProcessedElement> CreateProcessedElement(
      ElementId id, const ElementConverterOptions& options) override;

  void SetNumEvalPoints(int num_eval_points);

 private:
  int num_eval_points_ = 20;
  proto::Path unsafe_path_;
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_BEZIER_PATH_CONVERTER_H_
