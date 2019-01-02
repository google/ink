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

#include "ink/engine/brushes/brushes.h"

#include <cstdint>

#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/line/tip_type.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/security.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

using glm::vec2;
using glm::vec3;
using glm::vec4;
using status::InvalidArgument;

vec2 BrushParams::TipShapeParams::GetRadius(float size) const {
  float high = (2 * size * size_ratio) / (size_ratio + 1);
  float low = high / size_ratio;
  return vec2(low, high);
}

BrushParams::BrushParams()
    : animated(false),
      rgba_from(0, 0, 0, 1),
      rgba_seconds(0.0),
      dilation_from(1.0f),
      dilation_seconds(0.0),
      line_modifier(LineModifier::NONE),
      show_input_feedback(false),
      tip_type(TipType::Round) {}

void BrushParams::BrushSize::SetSize(float size, BrushSizeType size_type) {
  size_ = size;
  size_type_ = size_type;
}

float BrushParams::BrushSize::WorldSize(const Camera& cam) {
  switch (size_type_) {
    case BrushSizeType::SCREEN_UNITS:
      return cam.ConvertDistance(size_, DistanceType::kScreen,
                                 DistanceType::kWorld);
    case BrushSizeType::DP_UNITS:
      return cam.ConvertDistance(size_, DistanceType::kDp,
                                 DistanceType::kWorld);
    case BrushSizeType::WORLD_UNITS:
    default:
      return size_;
  }
}

Status BrushParams::PopulateAnimationFromProto(
    const ink::proto::LinearPathAnimation& unsafe_proto,
    BrushParams* brush_params) {
  if (unsafe_proto.has_rgba_from() && unsafe_proto.has_rgba_seconds()) {
    // Disallow crazy-long animations
    if (unsafe_proto.rgba_seconds() < 0 || unsafe_proto.rgba_seconds() > 60) {
      return InvalidArgument("Invalid animation seconds $0",
                             unsafe_proto.rgba_seconds());
    }
    brush_params->animated = true;
    brush_params->rgba_from =
        UintToVec4RGBA(static_cast<uint32_t>(unsafe_proto.rgba_from()));
    brush_params->rgba_seconds = unsafe_proto.rgba_seconds();
  }
  if (unsafe_proto.has_dilation_from() && unsafe_proto.has_dilation_seconds()) {
    // Disallow crazy-long animations
    if (unsafe_proto.dilation_seconds() < 0 ||
        unsafe_proto.dilation_seconds() > 60) {
      return InvalidArgument("Invalid dilation seconds $0",
                             unsafe_proto.dilation_seconds());
    }
    brush_params->animated = true;
    brush_params->dilation_from = unsafe_proto.dilation_from();
    brush_params->dilation_seconds = unsafe_proto.dilation_seconds();
  }
  return OkStatus();
}

Status BrushParams::BrushSize::PopulateSizeFromProto(
    const ink::proto::LineSize& unsafe_proto, float screen_width, float ppi,
    float world_width, ink::proto::BrushType brush, bool pen_mode,
    BrushParams::BrushSize* brush_size) {
  if (unsafe_proto.stroke_width() < 0) {
    return InvalidArgument("Negative stroke width not allowed");
  }
  float input_radius = unsafe_proto.stroke_width() / 2.0f;
  switch (unsafe_proto.units()) {
    case ink::proto::LineSize::POINTS: {
      brush_size->size_ =
          (world_width / screen_width) * PointsToPixels(input_radius, ppi);
      brush_size->size_type_ = BrushSizeType::WORLD_UNITS;
      break;
    }
    case ink::proto::LineSize::ZOOM_INDEPENDENT_DP: {
      brush_size->size_ = input_radius;
      brush_size->size_type_ = BrushSizeType::DP_UNITS;
      break;
    }
    case ink::proto::LineSize::WORLD_UNITS:
      brush_size->size_ = input_radius;
      brush_size->size_type_ = BrushSizeType::WORLD_UNITS;
      break;
    case ink::proto::LineSize::PERCENT_WORLD: {
      if (unsafe_proto.stroke_width() > 1) {
        return InvalidArgument("Stroke size percent $0 too large",
                               unsafe_proto.stroke_width());
      }
      brush_size->size_ =
          (world_width / screen_width) *
          PercentToPixelSize(unsafe_proto.stroke_width(), ppi, brush, pen_mode,
                             unsafe_proto.use_web_sizes());
      brush_size->size_type_ = BrushSizeType::WORLD_UNITS;
      break;
    }
    case ink::proto::LineSize::PERCENT_ZOOM_INDEPENDENT:
      if (unsafe_proto.stroke_width() > 1) {
        return InvalidArgument("Stroke size percent $0 too large",
                               unsafe_proto.stroke_width());
      }
      brush_size->size_ =
          PercentToPixelSize(unsafe_proto.stroke_width(), ppi, brush, pen_mode,
                             unsafe_proto.use_web_sizes());
      brush_size->size_type_ = BrushSizeType::SCREEN_UNITS;
      break;
    default:
      ASSERT(false);
      return InvalidArgument("Missing size units");
  }
  return OkStatus();
}

float BrushParams::PointsToPixels(float points, float ppi) {
  static const float kPointsPerInch = 72;
  return points * ppi / kPointsPerInch;
}

float BrushParams::PercentToPixelSize(float percent, float ppi,
                                      proto::BrushType brush, bool pen_mode,
                                      bool web_sizes) {
  float width;
  if (brush == proto::BrushType::BALLPOINT ||
      (brush == proto::BrushType::BALLPOINT_IN_PEN_MODE_ELSE_MARKER &&
       pen_mode)) {
    width = 3.4 * percent * percent + 0.5;
  } else {
    // Equation meant to approximate preexisting stroke widths for toolbar.
    // Brushes are welcome to diverge from this curve when appropriate.
    if (web_sizes) {
      width = 38 * percent * percent + 2;
    } else {
      width = 26.5 * percent * percent + 0.5;
    }
    if (brush == proto::BrushType::INKPEN) {
      width *= 0.5f;
    } else if (brush == proto::BrushType::PENCIL) {
      width *= 0.5f;
    } else if (brush == proto::BrushType::ERASER) {
      width *= 5.0f;
    }
  }

  return PointsToPixels(width, ppi) / 2.0f;
}

BrushParams BrushParams::GetBrushParams(ink::proto::BrushType brush,
                                        bool pen_mode) {
  BrushParams params;
  switch (brush) {
    case proto::BrushType::CALLIGRAPHY:
      params.expand_small_strokes = true;
      params.shape_params.size_ratio = 9.0f;
      params.shape_params.radius_behavior =
          TipShapeParams::RadiusBehavior::SPEED;
      break;
    case proto::BrushType::INKPEN:
      params.shape_params.size_ratio = 0.5f;
      params.shape_params.speed_limit = 8;
      params.shape_params.radius_behavior =
          TipShapeParams::RadiusBehavior::SPEED;
      break;
    case proto::BrushType::BALLPOINT:
      SetToBallpoint(&params);
      break;
    case proto::BrushType::BALLPOINT_IN_PEN_MODE_ELSE_MARKER:
      if (pen_mode) {
        SetToBallpoint(&params);
      }  // Otherwise act as a marker, which is all default values.
      break;
    case proto::BrushType::MARKER:
      // All defaults!
      break;
    case proto::BrushType::ERASER:
      params.line_modifier = LineModifier::ERASER;
      params.show_input_feedback = true;
      params.shape_params.size_ratio = 15;
      break;
    case proto::BrushType::HIGHLIGHTER:
      params.line_modifier = LineModifier::HIGHLIGHTER;
      params.tip_type = TipType::Square;
      params.shape_params.taper_amount = 0.0f;
      // This brush is animated, but the animation is computed in
      // highlighter.cc, not pre-defined in the proto.
      break;
    case proto::BrushType::PENCIL:
      params.line_modifier = LineModifier::PENCIL;
      params.expand_small_strokes = true;
      params.shape_params.radius_behavior =
          TipShapeParams::RadiusBehavior::PRESSURE;
      break;
    case proto::BrushType::CHARCOAL:
      params.tip_type = TipType::Square;
      params.line_modifier = LineModifier::CHARCOAL;
      params.shape_params.size_ratio = 4.0f;
      params.shape_params.radius_behavior =
          TipShapeParams::RadiusBehavior::FIXED;
      params.shape_params.taper_amount = 0.0f;
      break;
    case proto::BrushType::CHISEL:
      params.tip_type = TipType::Chisel;
      params.shape_params.radius_behavior =
          TipShapeParams::RadiusBehavior::TILT;
      params.shape_params.size_ratio = 10.0f;
      break;
    default:
      SLOG(SLOG_ERROR, "Invalid brush type requested");
      ASSERT(false);
  }
  return params;
}

void BrushParams::SetToBallpoint(BrushParams* params) {
  params->line_modifier = LineModifier::BALLPOINT;
  params->shape_params.radius_behavior =
      TipShapeParams::RadiusBehavior::PRESSURE;
}
}  // namespace ink
