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

#ifndef INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_LINE_CONVERTER_H_
#define INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_LINE_CONVERTER_H_
#include <cstdint>  // for uint32_t
#include <memory>
#include <utility>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/brushes.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/processing/element_converters/element_converter.h"
#include "ink/engine/scene/data/common/input_points.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/types/element_id.h"

namespace ink {

struct TessellationParams {
  bool linearize_mesh_verts;
  bool linearize_combined_verts;
  bool use_endcaps_on_all_lines;
  string texture_uri;

  TessellationParams()
      : linearize_mesh_verts(false),
        linearize_combined_verts(false),
        use_endcaps_on_all_lines(false),
        texture_uri("") {}
};

// Converts a set of lines into a ProcessedElement.
// The output of a ProcessedElement should be considered in group space,
//
// There are multiple coordinate systems to consider:
// Inputs:
// The lines are in an aribitrary coordinate space, called (L)ine-Space
// The points are in an arbitrary coordinate space, called (P)oint-Space
// Note that we expect lines[0] to be defined and have a valid DownCamera
// associated such that lines[0].DownCamera().Iview == L-to-P transform.
class LineConverter : public IElementConverter {
 public:
  LineConverter(const std::vector<FatLine>& lines,  // in L-space.
                const glm::mat4& group_to_p_space,
                std::unique_ptr<InputPoints> input_points,  // in P-space
                const ShaderType shader_type,
                TessellationParams tessellation_params);

  // Disallow copy and assign.
  LineConverter(const LineConverter&) = delete;
  LineConverter& operator=(const LineConverter&) = delete;

  // Create a new ProcessedElement with the stored line information and
  // associate it with the given element id.
  std::unique_ptr<ProcessedElement> CreateProcessedElement(
      ElementId id, const ElementConverterOptions& options) override;

 private:
  std::vector<FatLine> lines_;
  glm::mat4 group_to_p_space_{1};
  std::unique_ptr<InputPoints> input_points_;
  ShaderType mesh_shader_type_;
  TessellationParams tessellation_params_;
};

}  // namespace ink
#endif  // INK_ENGINE_PROCESSING_ELEMENT_CONVERTERS_LINE_CONVERTER_H_
