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

#ifndef INK_ENGINE_GEOMETRY_TESS_TESSELLATED_LINE_H_
#define INK_ENGINE_GEOMETRY_TESS_TESSELLATED_LINE_H_

#include <utility>

#include "third_party/absl/types/optional.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"

namespace ink {

// A FatLine that is tessellated to create a mesh.
class TessellatedLine {
 public:
  // Creates a new TessellatedLine getting services from gl_resources.
  explicit TessellatedLine(std::shared_ptr<GLResourceManager> gl_resources)
      : gl_resources_(std::move(gl_resources)) {}

  // Clears the line and tessellation, and sets up a new line.
  void SetupNewLine(float min_screen_travel_threshold, TipType tip_type,
                    FatLine::VertAddFn vertex_callback,
                    const LineModParams& line_mod_params);

  // Clears the line and tessellation, and restarts from the end of the given
  // line, minimum travel threshold, tip type. If a vertex-added callback is
  // given, it will use that, otherwise that will also be taken from the given
  // line.
  //
  // Returns the bounding box of the joining segment between the lines.
  OptRect RestartFromBackOfLine(
      const FatLine& other, const LineModParams& line_mod_params,
      absl::optional<FatLine::VertAddFn> vertex_callback = absl::nullopt);

  // Sets the object matrix of the line's mesh.
  void SetObjectMatrix(const glm::mat4& object_matrix) {
    tessellator_.mesh_.object_matrix = object_matrix;
  }

  // Sets the tip size, stylus state, and turn vertices, the extrudes a point
  // (see FatLine::Extrude()).
  //
  // Returns the bounding box of any created segements, or absl::nullopt if no
  // vertices are created.
  OptRect Extrude(glm::vec2 new_pt, InputTimeS time, TipSizeScreen tip_size,
                  input::StylusState stylus_state, int n_turn_verts,
                  bool force);

  // Builds the end cap of the line (see Fatline::BuildEndCap()).
  //
  // Returns the bounding box of the created endcap, or absl::nullopt if no
  // endcap is created.
  OptRect BuildEndCap();

  // Clears the vertices on the line, and any cached tessellation.
  // WARNING: This does not clear the object matrix, the minimum travel
  // threshold, the tip type, the line modifier parameters, or the vertex-added
  // callback.
  void ClearVertices();

  // Sets the shader metadata which will be passed on to the generated mesh.
  void SetShaderMetadata(ShaderMetadata metadata) {
    tessellator_.mesh_.shader_metadata = metadata;
  }

  // Returns the tessellation of the line.
  // NOTE: This is lazily generated. If the line has not changed, this will
  // return the cached mesh.
  const Mesh& GetMesh() const;

  // Returns a reference to the underlying FatLine.
  const FatLine& Line() const { return line_; }

 private:
  std::shared_ptr<GLResourceManager> gl_resources_;

  // These are mutable so that the line can be tessellated lazily.
  mutable Tessellator tessellator_;
  mutable bool mesh_dirty_ = false;

  bool has_end_cap_ = false;
  FatLine line_;
  LineModParams params_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_TESS_TESSELLATED_LINE_H_
