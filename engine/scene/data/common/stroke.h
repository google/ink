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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_STROKE_H_
#define INK_ENGINE_SCENE_DATA_COMMON_STROKE_H_

#include <cstddef>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/engine/scene/data/common/input_points.h"
#include "ink/engine/scene/data/common/mesh_serializer.h"
#include "ink/engine/scene/types/element_attributes.h"
#include "ink/engine/scene/types/element_bundle.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/util/security.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

// C++ equivalent to ink::proto::Stroke (See elements.proto)
class Stroke {
 public:
  Stroke();  // Empty ctor for deserialization

  Stroke(const UUID& uuid, ShaderType shader_type, glm::mat4 obj_to_world);

  static ABSL_MUST_USE_RESULT Status
  ReadFromProto(const proto::ElementBundle& unsafe_bundle, Stroke* s);
  static ABSL_MUST_USE_RESULT Status
  ReadFromProto(const ElementBundle& unsafe_bundle, Stroke* s);
  static void WriteToProto(proto::Stroke* stroke_proto, const Stroke& stroke);
  static void WriteToProto(proto::Element* element_proto, const Stroke& stroke);

  size_t MeshCount() const;

  S_WARN_UNUSED_RESULT Status GetMesh(const IMeshReader& mesh_reader,
                                      size_t lod_index, Mesh* mesh) const;
  S_WARN_UNUSED_RESULT Status GetCoverage(size_t lod_index,
                                          float* max_coverage) const;
  // output format is x,y = world coords, z = time in seconds
  S_WARN_UNUSED_RESULT Status GetInputPoints(InputPoints* pts) const;
  ShaderType shader_type() const { return shader_type_; }
  glm::mat4 obj_to_world() const { return obj_to_world_; }

  proto::Stroke* MutableProto() { return &stroke_; }
  const proto::Stroke& Proto() const { return stroke_; }

 private:
  void SerializeMesh(const OptimizedMesh& mesh, int lod_index,
                     float lod_coverage);

 private:
  UUID uuid_;
  proto::Stroke stroke_;
  glm::mat4 obj_to_world_{1};
  ShaderType shader_type_;
};

}  // namespace ink

#endif  // INK_ENGINE_SCENE_DATA_COMMON_STROKE_H_
