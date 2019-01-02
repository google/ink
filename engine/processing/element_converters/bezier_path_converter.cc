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

#include "ink/engine/processing/element_converters/bezier_path_converter.h"

#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/brushes/brushes.h"
#include "ink/engine/brushes/tool_type.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/line/fat_line.h"
#include "ink/engine/geometry/line/tip_type.h"
#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/bezier.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/processing/element_converters/line_converter.h"
#include "ink/engine/processing/element_converters/mesh_converter.h"
#include "ink/engine/scene/data/common/input_points.h"
#include "ink/engine/scene/graph/element_notifier.h"
#include "ink/engine/util/casts.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/elements_portable_proto.pb.h"
#include "ink/proto/geometry_portable_proto.pb.h"

namespace ink {

// Reject protos that have more than this many fields.
static constexpr size_t kMaxReasonableArgumentSize = 20000;

// Reject any coordinate or radius larger than this.
static constexpr float kMaxWorldCoordinateMagnitude =
    std::numeric_limits<float>::max() / 1000;

// This constant seems to provide good enough numeric stability with no visible
// over-simplification artifacts.
static constexpr float kMinimumNormalizedVertexDistance = .002;

using glm::vec2;
using ink::proto::Path;
using std::unique_ptr;

BezierPathConverter::BezierPathConverter(const Path& unsafe_path)
    : unsafe_path_(unsafe_path) {}

namespace {
bool IsSafeNumber(float num, float radius) {
  return std::isfinite(num) && num <= kMaxWorldCoordinateMagnitude &&
         num >= -kMaxWorldCoordinateMagnitude &&
         num + radius <= kMaxWorldCoordinateMagnitude &&
         num + radius >= -kMaxWorldCoordinateMagnitude &&
         num - radius <= kMaxWorldCoordinateMagnitude &&
         num - radius >= -kMaxWorldCoordinateMagnitude;
}

bool IsSafeNumber(float num) {
  return std::isfinite(num) && num <= kMaxWorldCoordinateMagnitude &&
         num >= -kMaxWorldCoordinateMagnitude;
}

bool PopulateBezierForPath(const Path& path, Bezier* bezier) {
  if (path.segment_args_size() == 0) {
    SLOG(SLOG_ERROR, "Invalid path: No segment arguments.");
    return false;
  }
  if (path.segment_args_size() > kMaxReasonableArgumentSize ||
      path.segment_types_size() > kMaxReasonableArgumentSize ||
      path.segment_counts_size() > kMaxReasonableArgumentSize) {
    SLOG(SLOG_ERROR, "Invalid path: suspiciously large");
    return false;
  }

  if (path.segment_args_size() % 2 != 0) {
    SLOG(SLOG_ERROR, "Invalid path: Odd number of segment arguments.");
    return false;
  }
  if (path.segment_types_size() != path.segment_counts_size()) {
    SLOG(SLOG_ERROR,
         "Invalid path: Segment types size does not match segment counts size");
    return false;
  }
  int segmentType;
  for (int i = 0; i < path.segment_types_size(); ++i) {
    segmentType = path.segment_types(i);
    if (segmentType != Path::MOVE_TO && segmentType != Path::LINE_TO &&
        segmentType != Path::CURVE_TO && segmentType != Path::QUAD_TO &&
        segmentType != Path::CLOSE) {
      SLOG(SLOG_ERROR, "Invalid path: Unknown path segment type");
      return false;
    }
  }

  // First gather raw vertices with some sanity checking.
  float radius = SafeNumericCast<double, float>(path.radius());
  std::vector<vec2> vertices;
  for (int i = 0; i < path.segment_args_size(); i += 2) {
    const float x = SafeNumericCast<double, float>(path.segment_args(i));
    if (!IsSafeNumber(x, radius)) {
      SLOG(SLOG_ERROR, "Invalid path: bad coordinate");
      return false;
    }
    const float y = SafeNumericCast<double, float>(path.segment_args(i + 1));
    if (!IsSafeNumber(y, radius)) {
      SLOG(SLOG_ERROR, "Invalid path: bad coordinate");
      return false;
    }
    vec2 new_point_world(x, y);
    // Don't add vertices that don't have a minimum delta in world coordinates.
    if (vertices.empty() ||
        geometry::Distance(new_point_world, vertices.back()) >=
            kMinimumNormalizedVertexDistance) {
      vertices.emplace_back(new_point_world);
    }
  }

  // Scale into standard object coordinate space.
  Rect raw_envelope = geometry::Envelope(vertices);
  raw_envelope = Rect::CreateAtPoint(raw_envelope.Center(),
                                     raw_envelope.Width() + 2 * radius,
                                     raw_envelope.Height() + 2 * radius);
  raw_envelope = raw_envelope.ContainingRectWithAspectRatio(1);
  glm::mat4 m_norm = ink::PackedVertList::CalcTransformForFormat(
      raw_envelope, ink::VertFormat::uncompressed);
  // Reject if the matrix isn't invertible.
  float det = glm::determinant(m_norm);
  if (det == 0 || std::isnan(det)) {
    SLOG(SLOG_ERROR, "Invalid path: matrix is non-invertable");
    return false;
  }

  auto trfm = [&m_norm](const vec2& v) -> vec2 {
    return vec2(m_norm * glm::vec4(v.x, v.y, 0, 1));
  };
  std::transform(vertices.begin(), vertices.end(), vertices.begin(), trfm);
  bezier->SetTransform(glm::inverse(m_norm));

  if (path.segment_types_size() == 0) {
    // Treat the data as a polyline if no segment_types are specified.
    bezier->LineTo(vertices.front());
    for (size_t i = 1; i < vertices.size(); i++) {
      const auto& v = vertices[i];
      if (geometry::Distance(v, bezier->Tip()) >=
          kMinimumNormalizedVertexDistance) {
        bezier->LineTo(v);
      }
    }
    return true;
  }

  int vertex_index = 0;
  const auto vertex_count = vertices.size();
  for (int i = 0; i < path.segment_counts_size(); ++i) {
    int count = path.segment_counts(i);
    if (count > kMaxReasonableArgumentSize) {
      SLOG(SLOG_ERROR, "Invalid path segment count: suspiciously large");
      return false;
    }
    auto segment_type = path.segment_types(i);
    for (int j = 0; j < count; ++j) {
      switch (segment_type) {
        case Path::MOVE_TO:
        case Path::LINE_TO: {
          if (vertex_index >= vertex_count) {
            return false;
          }
          const auto& v = vertices[vertex_index];
          if (vertex_index == 0 || geometry::Distance(v, bezier->Tip()) >=
                                       kMinimumNormalizedVertexDistance) {
            if (segment_type == Path::MOVE_TO) {
              bezier->MoveTo(v);
            } else {
              bezier->LineTo(v);
            }
          }
          vertex_index++;
          break;
        }
        case Path::CURVE_TO: {
          if (vertex_index + 2 >= vertex_count) {
            return false;
          }
          const auto& cp1 = vertices[vertex_index];
          const auto& cp2 = vertices[vertex_index + 1];
          const auto& to = vertices[vertex_index + 2];
          if (vertex_index == 0 || geometry::Distance(to, bezier->Tip()) >=
                                       kMinimumNormalizedVertexDistance) {
            bezier->CurveTo(cp1, cp2, to);
          }
          vertex_index += 3;
          break;
        }
        case Path::QUAD_TO: {
          if (vertex_index + 1 >= vertex_count) {
            return false;
          }
          const auto& cp = vertices[vertex_index];
          const auto& to = vertices[vertex_index + 1];
          if (vertex_index == 0 || geometry::Distance(to, bezier->Tip()) >=
                                       kMinimumNormalizedVertexDistance) {
            bezier->CurveTo(cp, to);
          }
          vertex_index += 2;
          break;
        }
        case Path::CLOSE:
          bezier->Close();
          break;
        default:
          RUNTIME_ERROR("Saw unknown path segment type");
      }
    }
  }

  // Return true for success only if all of the vertices were consumed.
  // If we didn't consume all of the data, something went horribly wrong and
  // there was dropped data at the end.
  return vertex_index == vertex_count;
}

bool IsClosedPath(const Path& path) {
  int seg_types_size = path.segment_types_size();
  return seg_types_size > 0 &&
         path.segment_types(seg_types_size - 1) == Path::CLOSE;
}

unique_ptr<ProcessedElement> BuildLine(ElementId el_id, const Bezier& bezier,
                                       const Path& path) {
  // The line converter expects everything in screen space and, in fact, assumes
  // that the transform passed in is equal to the DownCamera for the first line.
  // Create a camera to get a screen transform, as the contract with
  // LineConverter expects a valid screen to world transform but doesn't care
  // what it is.
  Camera cam;
  // Force the world window to be 1:1 with the screen to limit potential
  // floating point artifacts.
  auto screen_dim = cam.ScreenDim();
  cam.SetWorldWindow(
      Rect(glm::vec2(0, 0), glm::vec2(screen_dim[0], screen_dim[1])));
  ASSERT(matrix_utils::GetAverageAbsScale(cam.WorldToScreen()) == 1);
  // Ensure that the path tip input is valid.
  // The tip in the proto is in terms of world coordinates. The tip radius
  // should be set in terms of screen coordinates for FatLine.
  auto tip_radius = SafeNumericCast<double, float>(path.radius());
  if (std::isnan(tip_radius) || tip_radius <= 0 ||
      tip_radius > kMaxWorldCoordinateMagnitude) {
    SLOG(SLOG_ERROR, "Illegal radius");
    return nullptr;
  }

  const TipSizeScreen tip({tip_radius, tip_radius});

  glm::vec4 color(0, 0, 0, 1);
  if (path.has_rgba()) {
    color = UintToVec4RGBA(path.rgba());
  }
  glm::vec4 premultiplied = RGBtoRGBPremultiplied(color);
  const auto vert_callback =
      [premultiplied](glm::vec2 center, float radius, InputTimeS time,
                      float pressure, Vertex* pt,
                      std::vector<Vertex>* pts) { pt->color = premultiplied; };

  TipType endcap;
  switch (path.end_cap()) {
    case Path::BUTT:
    case Path::SQUARE:
      endcap = TipType::Square;
      break;
    case Path::ROUND:
      endcap = TipType::Round;
      break;
    default:
      SLOG(SLOG_ERROR, "Saw unknown endcap type");
      endcap = TipType::Round;
  }

  // On a closed path, generally treat all points as "internal". For now, get
  // reasonable behavior by forcing the endcaps to be round. When we handle
  // LineJoin properties we need to use that.
  if (IsClosedPath(path)) {
    endcap = TipType::Round;
  }

  std::vector<FatLine> paths;
  const auto& object_to_world = bezier.Transform();
  const auto& world_to_screen = cam.WorldToScreen();
  const auto object_to_screen = world_to_screen * object_to_world;
  for (const std::vector<vec2>& vertices : bezier.Polyline()) {
    if (vertices.size() < 2) {
      SLOG(SLOG_ERROR, "Attempted to stroke with <2 vertices");
      continue;
    }
    paths.emplace_back();
    auto& line = paths.back();

    line.ClearVertices();
    line.SetTipSize(tip);
    line.SetVertCallback(vert_callback);

    line.SetMinScreenTravelThreshold(0.01);
    line.SetTipType(endcap);
    line.SetTurnVerts(40);
    for (const vec2& v : vertices) {
      // The line converter expects that the lines are in screen space, and that
      // the transform passed in is the screen to world transform. Change the
      // vertices to ensure that the assumption holds.
      const vec2 screen = geometry::Transform(v, object_to_screen);
      if (!(IsSafeNumber(screen.x) && IsSafeNumber(screen.y))) {
        SLOG(SLOG_ERROR,
             "bad object to world transform probably due to too-large stroke "
             "dimensions");
        return nullptr;
      }
      line.Extrude(screen, InputTimeS(0), false);
    }
    line.BuildEndCap();
  }
  if (paths.empty()) {
    SLOG(SLOG_ERROR, "Degenerate path");
    return nullptr;
  }
  paths[0].SetDownCamera(cam);

  TessellationParams tessellation_params;
  tessellation_params.linearize_mesh_verts = false;
  tessellation_params.linearize_combined_verts = false;
  tessellation_params.use_endcaps_on_all_lines = true;

  // The group to world transform is considered to be the identity matrix as
  // data that comes in from proto is assumed to be group-local from the start.
  // By setting the group to world transform to be the identity transform, the
  // final transform that will be associated with the processed element will be
  // the current object to group transform.
  glm::mat4 group_to_world_transform(1.0f);
  LineConverter line_converter(paths, group_to_world_transform,
                               std::unique_ptr<InputPoints>(new InputPoints()),
                               SingleColorShader, tessellation_params);
  return line_converter.CreateProcessedElement(el_id);
}

std::vector<std::vector<Vertex>> CreateVertexPolyline(const Bezier& bezier) {
  std::vector<std::vector<Vertex>> ret;
  for (const std::vector<vec2>& vec2line : bezier.Polyline()) {
    ret.emplace_back();
    std::vector<Vertex>& vertex_line = ret.back();
    for (const vec2& vec : vec2line) {
      vertex_line.push_back(Vertex(vec));
    }
  }
  return ret;
}

unique_ptr<ProcessedElement> BuildFill(ElementId id, const Bezier& bezier,
                                       const Path& path) {
  Tessellator tess;
  if (!tess.Tessellate(CreateVertexPolyline(bezier))) {
    SLOG(SLOG_ERROR, "could not tessellate, skipping");
    return nullptr;
  }

  Mesh mesh = tess.mesh_;
  mesh.object_matrix = bezier.Transform();
  glm::vec4 color = UintToVec4RGBA(path.fill_rgba());
  glm::vec4 premultiplied = RGBtoRGBPremultiplied(color);
  for (size_t i = 0; i < mesh.verts.size(); ++i) {
    mesh.verts[i].color = premultiplied;
  }
  if (!mesh.verts.empty()) {
    SLOG(SLOG_DATA_FLOW, "drawing fill with: $0 vertices, first at ($1, $2)",
         mesh.verts.size(), mesh.verts[0].position.x, mesh.verts[0].position.y);
    MeshConverter mesh_converter(SingleColorShader, mesh);
    return mesh_converter.CreateProcessedElement(id);
  } else {
    SLOG(SLOG_ERROR, "attempted to build a fill with no vertices, skipping.");
    return nullptr;
  }
}
}  // namespace

unique_ptr<ProcessedElement> BezierPathConverter::CreateProcessedElement(
    ElementId id) {
  if (unsafe_path_.segment_args_size() != 0) {
    Bezier bezier;
    bezier.SetNumEvalPoints(num_eval_points_);
    if (!PopulateBezierForPath(unsafe_path_, &bezier)) {
      return nullptr;
    }
    if (unsafe_path_.has_fill_rgba()) {
      return BuildFill(id, bezier, unsafe_path_);
    }
    if (unsafe_path_.has_rgba()) {
      return BuildLine(id, bezier, unsafe_path_);
    }
  }
  SLOG(SLOG_ERROR, "Path proto did not specify anything to add.");
  return nullptr;
}

void BezierPathConverter::SetNumEvalPoints(int num_eval_points) {
  num_eval_points_ = num_eval_points;
}
}  // namespace ink
