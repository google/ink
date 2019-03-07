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

#include "ink/engine/processing/element_converters/stroke_outline_converter.h"

#include <cmath>

#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/distance.h"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/algorithms/simplify.h"
#include "ink/engine/geometry/mesh/shader_type.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/tess/tessellator.h"

namespace ink {

// Prevent malicious memory exhaustion.
static constexpr uint32_t kArbitraryVertexCountLimit = 20000;

// How near each other are two successive normalized points permitted to be?
static constexpr float kMinimumNormalizedVertexDistance = .002;

// How close to colinear are 3 successive normalized points permitted to be?
static constexpr float kMinimumOutlineSequenceAngle = .0001;

StrokeOutlineConverter::StrokeOutlineConverter(
    const proto::StrokeOutline& unsafe_stroke_outline)
    : unsafe_stroke_outline_(unsafe_stroke_outline) {}

std::unique_ptr<ProcessedElement>
StrokeOutlineConverter::CreateProcessedElement(
    ElementId id, const ElementConverterOptions& options) {
  SLOG(SLOG_DATA_FLOW, "line processor async task");

  const proto::StrokeOutline& stroke = unsafe_stroke_outline_;
  if (stroke.x_size() != stroke.y_size()) {
    SLOG(SLOG_ERROR, "bad outline with mismatched x and y sizes");
    return nullptr;
  }
  if (!stroke.has_rgba()) {
    SLOG(SLOG_ERROR, "bad outline with no rgba color");
    return nullptr;
  }
  if (stroke.x_size() < 3) {
    SLOG(SLOG_ERROR, "bad outline with fewer than 3 vertices");
    return nullptr;
  }
  // Sanitize vertices.

  // First gather raw vertices with some sanity checking and filtering.
  std::vector<Vertex> vertices;
  if (isnan(stroke.x(0)) || isnan(stroke.y(0))) {
    SLOG(SLOG_ERROR, "nan in vertex");
    return nullptr;
  }
  vertices.emplace_back(glm::vec2(stroke.x(0), stroke.y(0)));
  for (size_t i = 1; i < stroke.x_size(); i++) {
    if (isnan(stroke.x(i)) || isnan(stroke.y(i))) {
      SLOG(SLOG_ERROR, "nan in vertex");
      return nullptr;
    }
    glm::vec2 v(stroke.x(i), stroke.y(i));
    if (v != vertices.back().position) {
      vertices.emplace_back(v);
    }
  }

  // Normalize coordinates.
  Rect raw_envelope = geometry::Envelope(vertices);
  glm::mat4 m_norm = ink::PackedVertList::CalcTransformForFormat(
      raw_envelope, ink::VertFormat::x32y32);
  // Reject if the matrix isn't invertible.
  float det = glm::determinant(m_norm);
  if (det == 0 || std::isnan(det)) {
    SLOG(SLOG_ERROR, "matrix is non-inverseable");
    return nullptr;
  }
  auto trfm = [&m_norm](const Vertex& v) -> Vertex {
    return Vertex(m_norm * glm::vec4(v.position.x, v.position.y, 1, 1));
  };
  std::transform(vertices.begin(), vertices.end(), vertices.begin(), trfm);

  // Simplify
  {
    std::vector<Vertex> filtered;
    geometry::Simplify(vertices.begin(), vertices.end(), 0.1f,
                       std::back_inserter(filtered),
                       [](const Vertex& v) { return v.position; });
    vertices = std::move(filtered);
  }

  // Don't allow the last coordinate to be within minimum distance threshold of
  // the first.
  while (vertices.end() != vertices.begin() &&
         geometry::Distance(vertices.back(), vertices.front()) <
             kMinimumNormalizedVertexDistance) {
    vertices.pop_back();
  }
  // Close the path.
  if (!vertices.empty()) {
    vertices.emplace_back(vertices[0]);
  }

  const size_t outline_size = vertices.size();
  if (outline_size < 3) {
    SLOG(SLOG_ERROR, "bad outline with fewer than 3 vertices");
    return nullptr;
  }
  if (outline_size > kArbitraryVertexCountLimit) {
    SLOG(SLOG_ERROR,
         "not going to attempt to create outline with $0 vertices, which is "
         "more than kArbitraryVertexCountLimit of $1",
         outline_size, kArbitraryVertexCountLimit);
    return nullptr;
  }

  // Reject if 3 successive points are colinear.
  for (size_t i = 0; i < vertices.size() - 2; i++) {
    const auto& a = vertices[i].position;
    const auto& b = vertices[i + 1].position;
    const auto& c = vertices[i + 2].position;
    if (glm::distance(a, c) < kMinimumNormalizedVertexDistance) {
      SLOG(SLOG_ERROR, "bad outline with spike");
      return nullptr;
    }
    glm::vec3 v = glm::normalize(glm::vec3(b - a, 0));
    glm::vec3 w = glm::normalize(glm::vec3(c - b, 0));
    const auto angle = glm::acos(glm::dot(v, w));  // glm gives [0,PI]
    if (std::abs(angle - glm::pi<float>()) < kMinimumOutlineSequenceAngle) {
      SLOG(SLOG_ERROR, "bad outline with colinear 3-point sequence");
      return nullptr;
    }
  }

  Tessellator tess;
  if (!tess.Tessellate(vertices)) {
    SLOG(SLOG_ERROR, "could not tessellate");
    return nullptr;
  }
  if (tess.mesh_.verts.size() < 3) {
    SLOG(SLOG_ERROR, "tessellator produced mesh with fewer than 3 vertices");
    return nullptr;
  }
  tess.mesh_.object_matrix = glm::inverse(m_norm);
  tess.mesh_.verts[0].color =
      RGBtoRGBPremultiplied(UintToVec4RGBA(stroke.rgba()));

  auto processed_element = absl::make_unique<ProcessedElement>(
      id, tess.mesh_, ShaderType::SingleColorShader, options.low_memory_mode);
  // Preserve outlines on generated elements.
  for (const auto Vertex : vertices) {
    processed_element->outline.emplace_back(Vertex.position);
  }
  return processed_element;
}

}  // namespace ink
