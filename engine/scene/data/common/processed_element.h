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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_PROCESSED_ELEMENT_H_
#define INK_ENGINE_SCENE_DATA_COMMON_PROCESSED_ELEMENT_H_

#include <algorithm>
#include <memory>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/scene/data/common/input_points.h"
#include "ink/engine/scene/data/common/stroke.h"
#include "ink/engine/scene/types/element_attributes.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/scene/types/text.h"
#include "ink/engine/util/security.h"

namespace ink {

struct ProcessedElement {
  ElementId id;
  GroupId group = kInvalidElementId;    // The group id. kInvalidElementId
                                        // represents the root.
  std::unique_ptr<OptimizedMesh> mesh;  // object coordinates
  InputPoints input_points;             // x, y, t_sec in object coordinates
  std::vector<glm::vec2> outline;       // x, y in object coordinates
  glm::mat4 obj_to_group{1};            // Relative transform to the group.
  ElementAttributes attributes;
  std::unique_ptr<text::TextSpec> text;

  // This factory function creates a ProcessedElement, populating the mesh,
  // obj_to_group matrix, and input points (if present) from the given stroke.
  // NOTE: If the stroke has more than one mesh (from the deprecated
  // level-of-detail logic), the mesh with the best coverage will be chosen.
  static S_WARN_UNUSED_RESULT Status
  Create(ElementId id, const Stroke& stroke, ElementAttributes attributes,
         std::unique_ptr<ProcessedElement>* out);

  // Constructs a ProcessedElement with a mesh. The mesh field will be
  // constructed from mesh_in and shader_type_in, and the obj_to_group matrix
  // will be populated from the mesh field's object matrix.
  ProcessedElement(ElementId id_in, const Mesh& mesh_in,
                   ShaderType shader_type_in,
                   ElementAttributes attributes_in = ElementAttributes());
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_DATA_COMMON_PROCESSED_ELEMENT_H_
