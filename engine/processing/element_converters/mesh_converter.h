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

#ifndef INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_MESH_CONVERTER_H_
#define INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_MESH_CONVERTER_H_

#include <memory>

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

// Converts a mesh into a ProcessedElement.
class MeshConverter : public IElementConverter {
 public:
  // "shaderType" specifies the PackedVertShader that will be used to draw the
  // mesh and VertFormat that will be used to store data on the gpu, see
  // OptimizedMesh constructor for the conversion for ShaderType to VertFormat
  MeshConverter(ShaderType shader_type, const Mesh& mesh,
                ElementAttributes attributes = ElementAttributes())
      : shader_type_(shader_type), mesh_(mesh), attributes_(attributes) {
    EXPECT(!mesh_.verts.empty());
  }

  // Disallow copy and assign.
  MeshConverter(const MeshConverter&) = delete;
  MeshConverter& operator=(const MeshConverter&) = delete;

  std::unique_ptr<ProcessedElement> CreateProcessedElement(
      ElementId id) override {
    return absl::make_unique<ProcessedElement>(id, mesh_, shader_type_,
                                               attributes_);
  }

 private:
  ShaderType shader_type_;
  Mesh mesh_;
  ElementAttributes attributes_;
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_MESH_CONVERTER_H_
