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

#include "ink/engine/geometry/shape/shape.h"

#include <algorithm>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/geometry/primitives/circle_utils.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/geometry/tess/cdrefinement.h"
#include "ink/engine/math_defines.h"
#include "ink/engine/rendering/gl_managers/mesh_vbo_provider.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/funcs/utils.h"

namespace ink {

std::vector<Vertex> ShapeGeometry::GenVerts(glm::vec2 center, glm::vec2 size,
                                            glm::vec4 color) const {
  std::vector<Vertex> res;
  switch (type) {
    case ShapeGeometry::Type::Circle:
      res.reserve(40);
      for (const auto& p : PointsOnCircle(center, size.x / 2.0f, 40, 0, M_TAU))
        res.push_back(Vertex(p));
      break;
    case ShapeGeometry::Type::Rectangle:
      res.reserve(4);
      for (auto corner : RotRect(center, size, 0).Corners())
        res.emplace_back(corner);
      break;
  }

  for (auto& v : res) v.color = color;
  return res;
}

///////////////////////////////////////////////////////////////////////////////

Shape::Shape(ShapeGeometry shape_geometry)
    : border_visible_(true),
      fill_visible_(true),
      overall_visible_(true),
      outer_border_rgba_(0, 0, 0, 1),
      inner_border_rgba_(0, 0, 0, 1),
      fill_rgba_(1, 1, 1, 1),
      fill_size_world_(0),
      border_size_world_(0),
      shape_geometry_(shape_geometry),
      fill_dirty_(true),
      border_dirty_(true) {}

bool Shape::GetBorderMesh(const GLResourceManager& resource_manager,
                          const Mesh** mesh) const {
  *mesh = nullptr;
  if (!border_dirty_) {
    *mesh = &border_mesh_;
    return true;
  }
  if (border_size_world_.x <= 0 || border_size_world_.y <= 0) return false;
  if (fill_size_world_.x <= 0 || fill_size_world_.y <= 0) return false;

  tess_.ClearGeometry();
  auto outerpts = shape_geometry_.GenVerts(
      glm::vec2(0), fill_size_world_ + border_size_world_ * 2.0f,
      outer_border_rgba_);
  auto innerpts = shape_geometry_.GenVerts(glm::vec2(0), fill_size_world_,
                                           inner_border_rgba_);

  std::reverse(innerpts.begin(), innerpts.end());
  std::vector<std::vector<Vertex>> pts;
  pts.push_back(outerpts);
  pts.push_back(innerpts);
  if (!tess_.Tessellate(pts)) {
    return false;
  }

  border_mesh_ = tess_.mesh_;
  if (inner_border_rgba_ != outer_border_rgba_) {
    CDR cdr(&border_mesh_);
    cdr.RefineMesh();
  }
  resource_manager.mesh_vbo_provider->ReplaceVBO(&border_mesh_,
                                                 GL_DYNAMIC_DRAW);
  SetPositionMatrices();
  border_dirty_ = false;
  *mesh = &border_mesh_;
  return true;
}

bool Shape::GetFillMesh(const GLResourceManager& resource_manager,
                        const Mesh** mesh) const {
  *mesh = nullptr;
  if (!fill_dirty_) {
    *mesh = &fill_mesh_;
    return true;
  }
  if (fill_size_world_.x <= 0 || fill_size_world_.y <= 0) return false;

  tess_.ClearGeometry();
  auto pts =
      shape_geometry_.GenVerts(glm::vec2(0), fill_size_world_, fill_rgba_);
  if (!tess_.Tessellate(pts)) {
    return false;
  }

  fill_mesh_ = tess_.mesh_;
  resource_manager.mesh_vbo_provider->ReplaceVBO(&fill_mesh_, GL_DYNAMIC_DRAW);
  SetPositionMatrices();
  fill_dirty_ = false;
  *mesh = &fill_mesh_;
  return true;
}

void Shape::SetPositionMatrices() const {
  fill_mesh_.object_matrix =
      glm::translate(glm::mat4{1}, glm::vec3(center_world_, 0));
  border_mesh_.object_matrix =
      glm::translate(glm::mat4{1}, glm::vec3(center_world_, 0));
}

void Shape::SetBorderColor(glm::vec4 rgba_non_premultiplied) {
  SetBorderColor(rgba_non_premultiplied, rgba_non_premultiplied);
}
void Shape::SetBorderColor(glm::vec4 inner_border_rgba_non_premultiplied,
                           glm::vec4 outer_border_rgba_non_premultiplied) {
  outer_border_rgba_ =
      RGBtoRGBPremultiplied(outer_border_rgba_non_premultiplied);
  inner_border_rgba_ =
      RGBtoRGBPremultiplied(inner_border_rgba_non_premultiplied);
  border_dirty_ = true;
}
void Shape::SetFillColor(glm::vec4 rgba_non_premultiplied) {
  fill_rgba_ = RGBtoRGBPremultiplied(rgba_non_premultiplied);
  fill_dirty_ = true;
}
void Shape::SetFillSize(glm::vec2 world_size) {
  fill_size_world_ = world_size;
  fill_dirty_ = true;
  border_dirty_ = true;
}
void Shape::SetBorderSize(glm::vec2 world_size) {
  border_size_world_ = world_size;
  border_dirty_ = true;
}
void Shape::SetSizeAndPosition(Rect world_rect) {
  static const glm::vec2 no_border(0);
  SetSizeAndPosition(world_rect, no_border, false);
}
void Shape::SetSizeAndPosition(Rect world_rect, glm::vec2 border_size,
                               bool inset_border) {
  SetBorderSize(border_size);
  if (inset_border) {
    world_rect = world_rect.Inset(border_size_world_);
  }
  SetPosition(world_rect.Center());
  SetFillSize(world_rect.Dim());
}
void Shape::SetPosition(glm::vec2 world_center) {
  center_world_ = world_center;
  SetPositionMatrices();
}

glm::vec2 Shape::FillSize() const { return fill_size_world_; }

glm::vec2 Shape::BorderSize() const { return border_size_world_; }

glm::vec2 Shape::OverallSize() const {
  return fill_size_world_ + glm::vec2(2) * border_size_world_;
}

glm::vec2 Shape::WorldCenter() const { return center_world_; }

void Shape::SetVisible(bool visible) { overall_visible_ = visible; }
void Shape::SetBorderVisible(bool visible) { border_visible_ = visible; }
void Shape::SetFillVisible(bool visible) { fill_visible_ = visible; }

bool Shape::visible() const { return overall_visible_; }
bool Shape::fillVisible() const { return visible() && fill_visible_; }
bool Shape::borderVisible() const { return visible() && border_visible_; }
}  // namespace ink
