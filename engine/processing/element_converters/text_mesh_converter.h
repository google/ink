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

#ifndef INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_TEXT_MESH_CONVERTER_H_
#define INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_TEXT_MESH_CONVERTER_H_

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/text.h"

namespace ink {

// Basically the same as MeshConverter, but also sets the text field in
// ProcessedElement and sets the text ElementAttribute.
class TextMeshConverter : public IElementConverter {
 public:
  TextMeshConverter(const Mesh& mesh, const text::TextSpec text);

  std::unique_ptr<ProcessedElement> CreateProcessedElement(
      ElementId id) override;

  // Disallow copy and assign.
  TextMeshConverter(const TextMeshConverter&) = delete;
  TextMeshConverter& operator=(const TextMeshConverter&) = delete;

 private:
  Mesh mesh_;
  text::TextSpec text_;
};

}  // namespace ink

#endif  // INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_TEXT_MESH_CONVERTER_H_
