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

#ifndef INK_ENGINE_REALTIME_LINE_BUILDER_H_
#define INK_ENGINE_REALTIME_LINE_BUILDER_H_

#include <memory>
#include <vector>

#include "ink/engine/brushes/tip_dynamics.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/tess/tessellated_line.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/input/modeled_input.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/realtime/modifiers/line_modifier_factory.h"
#include "ink/engine/service/unchecked_registry.h"

namespace ink {

// The LineBuilder is repsonsible for constructing the stroke mesh from modeled
// input and prediction. It consists of three parts:
// - Stable lines: The portions of the line that have been built. "Stable"
//   refers to the fact that these will not change if additional points are
//   extruded.
// - Unstable line: The portion of the line that is currently being built.
//   "Unstable" refers to the face that outlines may be simplified when new
//   points are extruded, removing points are close to collinear. When the
//   unstable line becomes too long, a "split" occurs: the unstable line is
//   added to the stable lines (becoming immutable), and a new unstable line
//   picks up where the previous one left off.
// - Predicted line: This is the line constructed from the model's prediction.
//   It is thrown away whenever we extrude a new point, and is never added to
//   the stable lines.
class LineBuilder {
 public:
  explicit LineBuilder(std::shared_ptr<settings::Flags> flags,
                       std::shared_ptr<GLResourceManager> gl_resources)
      : flags_(std::move(flags)),
        gl_resources_(std::move(gl_resources)),
        unstable_line_(gl_resources_),
        predicted_line_(gl_resources_) {}

  void SetShaderMetadata(const ShaderMetadata& metadata) {
    unstable_line_.SetShaderMetadata(metadata);
    predicted_line_.SetShaderMetadata(metadata);
    stable_mesh_.shader_metadata = metadata;
  }

  // Clears the lines and their tessellations, and sets up new ones.
  void SetupNewLine(const Camera& down_camera, TipType tip_type,
                    InputTimeS start_time, input::InputType input_type,
                    std::unique_ptr<LineModifier> modifier,
                    std::unique_ptr<LineModifier> prediction_modifier);

  // Clears all lines and their tessellations.
  // WARNING: After clearing, you must call SetupNewLine() before extruding or
  // constructing a prediction.
  void Clear();

  // Extrudes each of the modeled inputs. If the unstable segment has grown too
  // long, it will split the line, adding the unstable segment to the completed
  // segments list and starting a new unstable segment from where it left off.
  // If is_line_end is true, it will build an end cap, complete the unstable
  // segment, and call LineModifier::ModifyFinalLine().
  // Note that this also clears the prediction.
  //
  // Returns the screen bounding box of any created segments, or absl::nullopt
  // if no vertices were created.
  OptRect ExtrudeModeledInput(const Camera& cam,
                              const std::vector<input::ModeledInput>& modeled,
                              bool is_line_end);

  // Constructs the prediction, building from the last extruded point.
  //
  // Returns the screen bounding box of the predicted segments, or absl::nullopt
  // if no prediction was created.
  OptRect ConstructPrediction(
      const Camera& cam,
      const std::vector<input::ModeledInput>& prediction_points);

  // Accessors for the line meshes.
  const Mesh& StableMesh() const { return stable_mesh_; }
  const Mesh& UnstableMesh() const { return unstable_line_.GetMesh(); }
  const Mesh& PredictionMesh() const { return predicted_line_.GetMesh(); }

  // The total number of midpoints in the completed segments and unstable
  // segment.
  int MidPointCount() const;

  std::vector<MidPoint> MostRecentCompletedMidPoints() const {
    return completed_lines_.empty() ? std::vector<MidPoint>()
                                    : completed_lines_.back().MidPoints();
  }
  std::vector<MidPoint> UnstableMidPoints() const {
    return unstable_line_.Line().MidPoints();
  }
  std::vector<MidPoint> PredictionMidPoints() const {
    return predicted_line_.Line().MidPoints();
  }

  const Camera& DownCamera() const { return down_camera_; }

  const std::vector<FatLine>& CompletedLines() const {
    return completed_lines_;
  }
  LineModifier* GetLineModifier() const { return modifier_.get(); }

 private:
  // Returns a pointer to the most recent non-empty line, which may be either
  // the unstable line or the last completed line. If the unstable line
  // is empty and there are no completed lines, it instead returns nullptr.
  const FatLine* LastNonEmptyLine() const;

  // Adds the unstable segment to the completed segments list, and starts a new
  // unstable segment from where it left off.
  void SplitLine();

  // The predicted segment sets per-vertex opacity based on prediction
  // confidence (actual + perceived) so the shift in line location is not as
  // apparent.
  float ModifyVertexOpacity(input::InputType input_type, glm::vec2 position);

  std::shared_ptr<settings::Flags> flags_;
  std::shared_ptr<GLResourceManager> gl_resources_;

  Camera down_camera_;

  TessellatedLine unstable_line_;
  TessellatedLine predicted_line_;
  std::unique_ptr<LineModifier> modifier_;
  std::unique_ptr<LineModifier> prediction_modifier_;
  FatLine::VertAddFn vertex_callback_;
  FatLine::VertAddFn prediction_vertex_callback_;

  std::vector<FatLine> completed_lines_;
  Mesh stable_mesh_;
};

}  // namespace ink

#endif  // INK_ENGINE_REALTIME_LINE_BUILDER_H_
