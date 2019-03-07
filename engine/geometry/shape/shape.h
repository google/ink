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

#ifndef INK_ENGINE_GEOMETRY_SHAPE_SHAPE_H_
#define INK_ENGINE_GEOMETRY_SHAPE_SHAPE_H_

#include <vector>

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/vertex.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/geometry/tess/tessellator.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"

namespace ink {

class ShapeGeometry {
 public:
  enum class Type {
    Circle,
    Rectangle,
  } type;

  ShapeGeometry(Type type) : type(type) {}

  std::vector<Vertex> GenVerts(glm::vec2 center, glm::vec2 size,
                               float rotation_radians, glm::vec4 color) const;
};

// A filled shape, outline, and associated mesh.
//
// The outline can have an inner and outer color, and the fill is always solid.
class Shape {
 public:
  explicit Shape(ShapeGeometry shape_geometry);

  bool visible() const;
  bool fillVisible() const;
  bool borderVisible() const;

  // Returns true if the mesh can be safely drawn.
  bool GetBorderMesh(const GLResourceManager& resource_manager,
                     const Mesh** mesh) const;
  bool GetFillMesh(const GLResourceManager& resource_manager,
                   const Mesh** mesh) const;

  void SetFillColor(glm::vec4 rgba_non_premultiplied);

  // Differing inner/outer colors will set the border to a gradient
  void SetBorderColor(glm::vec4 rgba_non_premultiplied);
  void SetBorderColor(glm::vec4 inner_border_rgba_non_premultiplied,
                      glm::vec4 outer_border_rgba_non_premultiplied);

  // Set the size and position of this shape with no border.
  void SetSizeAndPosition(Rect world_rect);
  void SetSizeAndPosition(RotRect world_rect);

  // If inset_border: The border is added to the inside of this shape
  // else:            The border is added to the outside of this shape
  void SetSizeAndPosition(Rect world_rect, glm::vec2 border_size,
                          bool inset_border);
  void SetSizeAndPosition(RotRect world_rect, glm::vec2 border_size,
                          bool inset_border);

  // Overall width = fillSize.x + 2*border_size.x;
  // Overall height = fillSize.y + 2*border_size.y;
  void SetFillSize(glm::vec2 world_size);
  void SetBorderSize(glm::vec2 world_size);
  void SetPosition(glm::vec2 world_center);
  void SetRotation(float radians);

  glm::vec2 FillSize() const;
  glm::vec2 BorderSize() const;
  glm::vec2 OverallSize() const;
  glm::vec2 WorldCenter() const;
  float Rotation() const;

  void SetVisible(bool visible);
  void SetBorderVisible(bool visible);
  void SetFillVisible(bool visible);

 private:
  void SetPositionMatrices() const;

 private:
  bool border_visible_;
  bool fill_visible_;
  bool overall_visible_;

  glm::vec4 outer_border_rgba_{0, 0, 0, 0};  // premultiplied
  glm::vec4 inner_border_rgba_{0, 0, 0, 0};  // premultiplied
  glm::vec4 fill_rgba_{0, 0, 0, 0};          // premultiplied

  glm::vec2 fill_size_world_{0, 0};
  glm::vec2 border_size_world_{0, 0};
  glm::vec2 center_world_{0, 0};
  float rotation_radians_ = 0.0f;

  ShapeGeometry shape_geometry_;

  // Cache meshes between draw calls
  mutable Tessellator tess_;
  mutable bool fill_dirty_;
  mutable Mesh fill_mesh_;
  mutable bool border_dirty_;
  mutable Mesh border_mesh_;
};

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_SHAPE_SHAPE_H_
