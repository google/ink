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

#include "ink/engine/processing/element_converters/text_mesh_converter.h"
#include "third_party/absl/memory/memory.h"

namespace ink {

TextMeshConverter::TextMeshConverter(const Mesh& mesh,
                                     const text::TextSpec text)
    : mesh_(mesh), text_(text) {}

std::unique_ptr<ProcessedElement> TextMeshConverter::CreateProcessedElement(
    ElementId id) {
  auto element = absl::make_unique<ProcessedElement>(
      id, mesh_, ShaderType::TexturedVertShader);
  element->attributes.is_text = true;
  element->text = absl::make_unique<text::TextSpec>(text_);
  return element;
}

}  // namespace ink
