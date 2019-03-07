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

#ifndef INK_ENGINE_REALTIME_LINE_TOOL_H_
#define INK_ENGINE_REALTIME_LINE_TOOL_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "testing/production_stub/public/gunit_prod.h"  // Defines FRIEND_TEST
#include "third_party/absl/types/optional.h"
#include "ink/engine/brushes/brushes.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/shape/shape.h"
#include "ink/engine/geometry/tess/tessellated_line.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/input/cursor.h"
#include "ink/engine/input/input_handler.h"
#include "ink/engine/input/input_modeler.h"
#include "ink/engine/public/types/color.h"
#include "ink/engine/realtime/line_builder.h"
#include "ink/engine/realtime/line_tool_data_sink.h"
#include "ink/engine/realtime/modifiers/line_modifier.h"
#include "ink/engine/realtime/modifiers/line_modifier_factory.h"
#include "ink/engine/realtime/particle_builder.h"
#include "ink/engine/realtime/tool.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/renderers/mesh_renderer.h"
#include "ink/engine/rendering/renderers/shape_renderer.h"
#include "ink/engine/scene/data/common/input_points.h"
#include "ink/engine/scene/frame_state/frame_state.h"
#include "ink/engine/scene/layer_manager.h"
#include "ink/engine/scene/page/page_bounds.h"
#include "ink/engine/scene/page/page_manager.h"
#include "ink/engine/service/unchecked_registry.h"
#include "ink/engine/util/dbg_helper.h"
#include "ink/engine/util/dbg_input_visualizer.h"
#include "ink/engine/util/time/time_types.h"

namespace ink {

// LineTool takes in input data and renders a stroke as it is being drawn.
// When the line is completed, the resulting FatLine and other data are sent
// to the LineToolDataSync.
class LineTool : public Tool {
 public:
  explicit LineTool(const service::UncheckedRegistry& registry,
                    InputRegistrationPolicy input_registration_policy =
                        InputRegistrationPolicy::ACTIVE);
  input::CaptureResult OnInput(const input::InputData& data,
                               const Camera& live_camera) override;
  absl::optional<input::Cursor> CurrentCursor(
      const Camera& camera) const override;
  void Draw(const Camera& live_camera, FrameTimeS draw_time) const override;
  void Update(const Camera& camera, FrameTimeS draw_time) override;
  void Enable(bool enabled) override;
  void Clear();
  void EnableDebugMesh(bool enabled);

  const BrushParams& GetBrushParams() const { return brush_params_; }
  void SetBrushParams(BrushParams params);
  void SetColor(glm::vec4 rgba) override;
  glm::vec4 GetColor() override { return rgba_; }
  int MidPointCount() const { return line_builder_.MidPointCount(); }
  std::vector<MidPoint> MostRecentCompletedMidPoints() const {
    return line_builder_.MostRecentCompletedMidPoints();
  }
  std::vector<MidPoint> UnstableMidPoints() const {
    return line_builder_.UnstableMidPoints();
  }
  std::vector<MidPoint> PredictionMidPoints() const {
    return line_builder_.PredictionMidPoints();
  }

  // Longform support
  // Are we actively drawing / creating a line?
  bool IsDrawing() const;

  // Longform support
  // MBR of the input making up the current line. 0 if !IsDrawing()
  Rect InputRegion() const;

  // Returns the MBR of any new geometry generated since the start of the line
  // or the last call to ResetUpdatedRegion.  Includes both the current and
  // previous predictions.
  OptRect UpdatedRegion() const { return updated_region_; }

  // Discard the stored MBR of any new geometry and only record the MBR of the
  // last prediction.
  void ResetUpdatedRegion();

  // For debugging and testing, set the color of the predicted line.
  void SetPredictedLineColor(const Color& color) {
    predicted_line_color_ = color;
  }
  // Use the line color as the predicted line color.
  void UnsetPredictedLineColor() { predicted_line_color_ = absl::nullopt; }

  inline std::string ToString() const override { return "<LineTool>"; }

 private:
  void UpdateShapeFeedback(const input::InputData& data, float world_radius,
                           const Camera& live_camera);
  void ExtrudeLine(const input::InputData& data, bool is_line_end);

  // Returns true if we should clear the line and refuse the rest of the input
  // stream.
  bool ShouldClearAndRefuseInput(const input::InputData& data) const;

  // Changes the input data such that it signifies an artificial touch-up.
  void ChangeToTUp(input::InputData* data);

  void SetupNewLine(const input::InputData& data, const Camera& live_camera);

  // Loads any textures needed by the given brush, if any.
  void LoadBrushTextures(const BrushParams params);

  void RegeneratePredictedLine();
  void SendCompleteLineToSink();
  ShaderType GetShaderType(BrushParams::LineModifier line_modifier) const;
  TessellationParams ConvertLineModParamsToTessellationParams(
      LineModParams line_mod_params);

  void DebugDrawTriangulation() const;
  void DebugDrawPrediction();

  glm::vec4 PredictedLinePremultipliedVec() const;

  std::shared_ptr<GLResourceManager> gl_resources_;
  std::shared_ptr<LineToolDataSink> result_sink_;
  std::shared_ptr<input::InputModeler> input_modeler_;
  std::shared_ptr<LineModifierFactory> line_modifier_factory_;
  std::shared_ptr<PageBounds> page_bounds_;
  std::shared_ptr<PageManager> page_manager_;
  std::shared_ptr<LayerManager> layer_manager_;
  std::shared_ptr<IDbgHelper> dbg_helper_;

  BrushParams brush_params_;
  bool sent_up_;
  bool has_touch_id_;
  uint32_t touch_id_;

  glm::vec4 rgba_{0, 0, 0, 0};
  // For testing. If unset, uses rgba_ to draw the predicted line.
  absl::optional<Color> predicted_line_color_;

  MeshRenderer renderer_;
  Rect input_region_;
  OptRect predicted_region_;
  OptRect updated_region_;

  LineBuilder line_builder_;
  ParticleBuilder particles_;

  TessellationParams final_tessellation_params_;
  bool should_init_shader_metadata_;
  Shape shape_feedback_;
  ShapeRenderer shape_feedback_renderer_;
  std::unique_ptr<InputPoints> input_points_;
  GroupId current_group_;
  bool dbg_mesh_enabled_ = false;

  size_t prediction_dbg_count_ = 0;
  glm::vec4 prediction_dbg_color_ = glm::vec4(0.9f, 0.4f, 0.0f, 0.5f);

  FRIEND_TEST(SEngineTest, clearRestoresToolConfig);
};

}  // namespace ink
#endif  // INK_ENGINE_REALTIME_LINE_TOOL_H_
