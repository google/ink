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

#include "ink/engine/rendering/renderers/zoomable_rect_renderer.h"

#include <algorithm>
#include <vector>

#include "third_party/absl/memory/memory.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/mesh/shape_helpers.h"
#include "ink/engine/rendering/zoom_spec.h"

namespace ink {

namespace {
class ZoomNode {
 public:
  ZoomNode(absl::string_view base_uri, int tile_size, const ZoomSpec& spec)
      : spec_(spec), texture_info_(TextureUri(base_uri, spec)) {}

  // Constructs a tree of all relevant zoom nodes.
  // Each node in a tree contains a ZoomSpec yielding a region of the target
  // rectangle, and pointers to its visible more-zoomed-in children. If all of a
  // nodes's children have textures, then that node can be skipped in
  // rendering.
  // All of the leaf nodes should be rendered, which will cause their textures
  // to be fetched.
  void Build(absl::string_view base_uri, const Camera& cam,
             TextureManager* texture_manager, size_t tile_size,
             const Rect& world_bounds, const Rect& visible_rect_world) {
    auto tile_bounds_world = spec_.Apply(world_bounds);
    const float screen_width = cam.ConvertDistance(
        tile_bounds_world.Width(), DistanceType::kWorld, DistanceType::kScreen);

    // Is my rendered tile size smaller than its display size in pixels?
    const bool need_zoom = screen_width > tile_size;

    // Is my texture already available?
    const bool have_texture = texture_manager->HasTexture(texture_info_);

    if (!need_zoom && !have_texture) {
      // We are zoomed in far enough at this point in the tree, but have no
      // texture. Initiate a fetch now.
      texture_manager->MaybeStartClientImageRequest(texture_info_.uri);
    }

    for (const auto& q : kAllQuadrants) {
      ZoomSpec zoomed = spec_.ZoomedInto(q);
      auto zoomed_tile_bounds_world = zoomed.Apply(world_bounds);
      // Add a child if it is visible and either (1) we need further zoom or (2)
      // my texture is not available yet but the kid's texture is (which happens
      // while zooming out).
      if (!geometry::Intersects(zoomed_tile_bounds_world, visible_rect_world)) {
        // This kid isn't on screen.
        continue;
      }
      if (need_zoom ||
          (!have_texture && texture_manager->HasTexture(
                                TextureInfo(TextureUri(base_uri, zoomed))))) {
        kids_.emplace_back(base_uri, tile_size, zoomed);
        kids_.back().Build(base_uri, cam, texture_manager, tile_size,
                           world_bounds, visible_rect_world);
      }
    }
  }

  void UpdateCoverage(const TextureManager& texture_manager) {
    if (kids_.empty()) {
      covered_ = texture_manager.HasTexture(texture_info_);
      return;
    }
    // If all my kids draw, do nothing.
    covered_ = true;
    for (auto& kid : kids_) {
      kid.UpdateCoverage(texture_manager);
      covered_ &= kid.covered_;
    }
    covered_ |= texture_manager.HasTexture(texture_info_);
  }

  // Will initiate texture fetches on all leaf nodes.
  void Render(const Camera& cam, const Rect& world_bounds,
              const GLResourceManager& gl_resources,
              OptimizedMesh* mesh) const {
    bool can_draw_all_kids =
        std::all_of(kids_.begin(), kids_.end(),
                    [](const ZoomNode& k) { return k.covered_; });

    // If I am a leaf, or I'm a non-leaf with incomplete child coverage and an
    // already-loaded texture, draw.
    if (kids_.empty() ||
        (!can_draw_all_kids &&
         gl_resources.texture_manager->HasTexture(texture_info_))) {
      const Rect& tile_rect_world = spec_.Apply(world_bounds);
      Texture* texture = nullptr;
      // This will initiate a texture fetch for leaf nodes.
      if (gl_resources.texture_manager->GetTexture(texture_info_, &texture)) {
        mesh->texture->Reset(texture_info_.uri);
        mesh->object_matrix = mesh->mbr.CalcTransformTo(tile_rect_world, true);
        auto& shader = gl_resources.shader_manager->PackedShader();
        shader.Use(cam, *mesh);
        shader.Draw(*mesh);
        shader.Unuse(*mesh);
      }
    }

    for (auto const& kid : kids_) {
      kid.Render(cam, world_bounds, gl_resources, mesh);
    }
  }

 private:
  ZoomSpec spec_;
  TextureInfo texture_info_;
  bool covered_ = false;
  std::vector<ZoomNode> kids_;

  static std::string TextureUri(absl::string_view base_uri,
                                const ZoomSpec& zoom_spec) {
    return absl::StrCat(base_uri, "?", zoom_spec.ToUriParam());
  }
};

}  // namespace

ZoomableRectRenderer::ZoomableRectRenderer(
    std::shared_ptr<GLResourceManager> gl_resources)
    : gl_resources_(std::move(gl_resources)),
      shape_renderer_(gl_resources_),
      rectangle_mesh_bounds_(0, 0, 1, 1),
      white_rect_(ShapeGeometry(ShapeGeometry::Type::Rectangle)) {
  Mesh m;
  MakeRectangleMesh(&m, rectangle_mesh_bounds_, glm::vec4{1}, glm::mat4(1));
  rectangle_mesh_ = absl::make_unique<OptimizedMesh>(TexturedVertShader, m);
  rectangle_mesh_->texture =
      absl::make_unique<TextureInfo>("I will be replaced with real URIs");
  gl_resources_->mesh_vbo_provider->EnsureOnlyInVBO(rectangle_mesh_.get(),
                                                    GL_STATIC_DRAW);
  white_rect_.SetFillColor(glm::vec4(1, 1, 1, 1));
  white_rect_.SetBorderVisible(false);
  white_rect_.SetFillVisible(true);
}

void ZoomableRectRenderer::Draw(const Camera& cam, FrameTimeS draw_time,
                                const Rect& object_worldspace_bounds,
                                absl::string_view base_texture_uri) const {
  // Find the intersection of the mesh with the visible part of the scene.
  Rect visible_mesh_world;
  if (!geometry::Intersection(cam.WorldWindow(), object_worldspace_bounds,
                              &visible_mesh_world)) {
    ION_LOG_EVERY_N_SEC(WARNING, 1)
        << "why am I rendering something not even on screen?\ncam:"
        << Str(cam.WorldWindow()) << "\nobj:" << object_worldspace_bounds;
    return;
  }

  // Draw a white rectangle over the visible part of the page. This provides a
  // more pleasant experience on slower platforms, where otherwise gray
  // background squares emphasize missing tiles.
  white_rect_.SetSizeAndPosition(visible_mesh_world);
  shape_renderer_.Draw(cam, draw_time, white_rect_);

  const size_t tile_dimension =
      gl_resources_->texture_manager->GetTilePolicy().tile_side_length;

  // Construct a tree of visible tiles, whose leaf nodes are the most zoomed-in
  // tiles required for this camera.
  ZoomNode root(base_texture_uri, tile_dimension, ZoomSpec());
  root.Build(base_texture_uri, cam, gl_resources_->texture_manager.get(),
             tile_dimension, object_worldspace_bounds, visible_mesh_world);
  root.UpdateCoverage(*(gl_resources_->texture_manager));
  root.Render(cam, object_worldspace_bounds, *gl_resources_,
              rectangle_mesh_.get());
}

}  // namespace ink
