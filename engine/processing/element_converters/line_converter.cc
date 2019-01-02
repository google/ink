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

#include "ink/engine/processing/element_converters/line_converter.h"

#include <algorithm>
#include <cstddef>  // for size_t
#include <memory>
#include <utility>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/tess/cdrefinement.h"
#include "ink/engine/geometry/tess/color_linearizer.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

LineConverter::LineConverter(const std::vector<FatLine>& lines,
                             const glm::mat4& group_to_p_space,
                             std::unique_ptr<InputPoints> input_points,
                             const ShaderType shader_type,
                             TessellationParams tessellation_params)
    : lines_(lines),
      group_to_p_space_(group_to_p_space),
      input_points_(std::move(input_points)),
      mesh_shader_type_(shader_type),
      tessellation_params_(tessellation_params) {}

std::unique_ptr<ProcessedElement> LineConverter::CreateProcessedElement(
    ElementId id) {
  SLOG(SLOG_DATA_FLOW, "line processor async task");

  Tessellator tess;
  Mesh mesh;
  for (size_t i = 0; i < lines_.size(); i++) {
    FatLine line = lines_[i];
    bool end_cap = tessellation_params_.use_endcaps_on_all_lines ||
                   (i == lines_.size() - 1);
    if (!tess.Tessellate(line, end_cap)) {
      SLOG(SLOG_ERROR, "could not tessellate");
      return nullptr;
    }

    if (tess.mesh_.idx.empty() && tess.mesh_.verts.empty()) {
      SLOG(SLOG_ERROR, "Degenerate mesh");
      return nullptr;
    }

    // Calculate the transform for screen -> object coordinates. If the
    // resulting matrix would be non-invertible (due to a very large or
    // very small scale factor), fail.
    auto envelope = geometry::Envelope(tess.mesh_.verts);
    VertFormat fmt = OptimizedMesh::VertexFormat(mesh_shader_type_);
    auto m = PackedVertList::CalcTransformForFormat(envelope, fmt);
    float det = glm::determinant(m);
    if (det == 0 || std::isnan(det) || std::isinf(det)) {
      SLOG(SLOG_ERROR, "Degenerate mesh: matrix is non-invertable");
      return nullptr;
    }

    tess.mesh_.object_matrix = glm::inverse(m);
    for (auto& v : tess.mesh_.verts) {
      v.position = geometry::Transform(v.position, m);
    }

    auto cdr = CDR(&tess.mesh_);
    cdr.RefineMesh();
    auto clr = ColorLinearizer(&tess.mesh_);

    if (tessellation_params_.linearize_combined_verts) {
      clr.LinearizeCombinedVerts();
    }
    if (tessellation_params_.linearize_mesh_verts) {
      clr.LinearizeAllVerts();
    }
    if (!tessellation_params_.texture_uri.empty()) {
      mesh.texture =
          absl::make_unique<TextureInfo>(tessellation_params_.texture_uri);
    }

    mesh.Append(tess.mesh_);
    tess.Clear();
  }

  // Set the mesh's transform. Note that the mesh is in L-Space. The L-to-P
  // transform is given by the camera as seen by pendown.
  mesh.object_matrix = lines_[0].DownCamera().ScreenToWorld();

  // This sets the processed element's obj to group, which is a function of the
  // mesh transform and the points in the mesh. We are temporarily lying about
  // what the obj to group transform is in the processed element, as what
  // is being stored is obj to p space.
  auto processed_element =
      absl::make_unique<ProcessedElement>(id, mesh, mesh_shader_type_);

  // Transform all the input points into object local coordinates.
  glm::mat4 p_space_to_obj = glm::inverse(processed_element->obj_to_group);
  input_points_->TransformPoints(p_space_to_obj);

  processed_element->input_points = *input_points_;
  processed_element->outline = FatLine::OutlineAsArray(lines_, p_space_to_obj);
  // Re-transform the obj to group transform to be group local.
  // Keep in mind that the mesh's object matrix will continue to be the obj to
  // p space transform.
  processed_element->obj_to_group =
      glm::inverse(group_to_p_space_) * processed_element->obj_to_group;
  return processed_element;
}

}  // namespace ink
