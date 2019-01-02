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

#ifndef INK_ENGINE_BRUSHES_BRUSHES_H_
#define INK_ENGINE_BRUSHES_BRUSHES_H_

#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/tool_type.h"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/line/tip_type.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

enum class BrushSizeType { WORLD_UNITS, SCREEN_UNITS, DP_UNITS };

template <>
inline std::string Str(const BrushSizeType& t) {
  switch (t) {
    case BrushSizeType::SCREEN_UNITS:
      return "SCREEN_UNITS";
    case BrushSizeType::DP_UNITS:
      return "DP_UNITS";
    case BrushSizeType::WORLD_UNITS:
    default:
      return "WORLD_UNITS";
  }
}

// BrushParams define all the parameters used to vary the behavior of LineTool
// in order to render different-looking strokes. These values are only expected
// to change when the tool selection, size, etc. change.
struct BrushParams {
  class BrushSize {
   public:
    // Return the size of the brush in world units, may change with zoom level.
    float WorldSize(const Camera& cam);

    void SetSize(float size, BrushSizeType size_type);

    // Populate BrushParams' size fields from LineSize proto. This should only
    // be called when the Line tool is selected, otherwise these values aren't
    // relevant.
    // Returns OkStatus if parsing succeeded.
    static ABSL_MUST_USE_RESULT Status PopulateSizeFromProto(
        const proto::LineSize& unsafe_proto, float screen_width, float ppi,
        float world_width, proto::BrushType brush, bool pen_mode,
        BrushParams::BrushSize* brush_size);

   private:
    float size_ = 12.0f;
    BrushSizeType size_type_ = BrushSizeType::WORLD_UNITS;
  } size;

  // Animation support
  bool animated = false;
  glm::vec4 rgba_from{0, 0, 0, 0};  // Animation start, non-premultiplied
  DurationS rgba_seconds;           // Duration of color animation
  float dilation_from;              // Begin dilation with this scale factor
  DurationS dilation_seconds;       // Duration of duration

  // Post finger-up, use expand-to-large-dot feature (e.g. for dotting "i"s
  // with small brushes)
  bool expand_small_strokes = false;

  // Parameters for determining the radius of a stroke based upon various input
  // data.
  struct TipShapeParams {
    float size_ratio = 1;
    float speed_limit = 200;  // maximum cm/s
    float base_speed = 0;     // cm/s
    // How much tapering to apply from the modeled input to the tip of the
    // predicted input. (0 = no tapering; 1 = max tapering)
    float taper_amount = 0.3;

    enum class RadiusBehavior {
      MIN_VALUE,    // Just for bounds-checking
      FIXED,        // Radius doesn't change
      SPEED,        // Radius is based upon speed
      PRESSURE,     // Radius is based upon pressure
      TILT,         // Radius is based upon stylus tilt if available
      ORIENTATION,  // Based upon the stylus orientation vs. stroke angle
      MAX_VALUE
    } radius_behavior = RadiusBehavior::FIXED;

    // Determine a min/max target range centered around (size *
    // radiusMultiplier).
    // When sizeRatio is 9 (calligraphy), inputs and outputs look like:
    // http://www.wolframalpha.com/input/?i=plot+%282*s%29+%2F+%289+%2B+1%29%2C++%282*9*s%29+%2F+%289+%2B+1%29%2C+s+from+s+%3D+0+to+s+%3D100
    // When sizeRatio is 0.5 (ink pen), inputs and outputs look like:
    // http://www.wolframalpha.com/input/?i=plot+%282*s%29+%2F+%28.5+%2B+1%29%2C++%282*0.5*s%29+%2F+%280.5+%2B+1%29%2C+s+from+s+%3D+0+to+s+%3D100
    glm::vec2 GetRadius(float size) const;
  } shape_params;

  enum class LineModifier : uint32_t {
    MIN_VALUE,  // Just for bounds-checking
    NONE,
    HIGHLIGHTER,
    ERASER,
    BALLPOINT,
    PENCIL,
    CHARCOAL,
    MAX_VALUE
  } line_modifier;

  // NOTE(gmadrid): This flag is experimental and under development and likely
  // to change form radically in the near future. Please do NOT use.
  bool particles = false;

  bool show_input_feedback;
  TipType tip_type;

  BrushParams();

  // Populate BrushParams' animation values from LinearPathAnimation proto.
  static ABSL_MUST_USE_RESULT Status PopulateAnimationFromProto(
      const ink::proto::LinearPathAnimation& unsafe_proto,
      BrushParams* brush_params);
  static BrushParams GetBrushParams(proto::BrushType brush, bool pen_mode);

 private:
  static void SetToBallpoint(BrushParams* params);

  static float PointsToPixels(float points, float ppi);
  // Given a size percent [0,1] and brush, return an appropriate pixel radius
  // for the brush.
  static float PercentToPixelSize(float percent, float ppi,
                                  proto::BrushType brush, bool pen_mode,
                                  bool web_sizes);
};

}  // namespace ink
#endif  // INK_ENGINE_BRUSHES_BRUSHES_H_
