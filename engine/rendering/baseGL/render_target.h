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

#ifndef INK_ENGINE_RENDERING_BASEGL_RENDER_TARGET_H_
#define INK_ENGINE_RENDERING_BASEGL_RENDER_TARGET_H_

#include <memory>
#include <string>

#include "third_party/absl/types/optional.h"
#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/camera/camera.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/rendering/baseGL/blit_attrs.h"
#include "ink/engine/rendering/baseGL/gpupixels.h"
#include "ink/engine/rendering/baseGL/textured_quad_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/rendering/gl_managers/texture_params.h"

namespace ink {

enum class AntialiasingStrategy { kNone = 0, kMSAA = 1 };

template <>
inline std::string Str(const AntialiasingStrategy& t) {
  switch (t) {
    case AntialiasingStrategy::kNone:
      return "None";
    case AntialiasingStrategy::kMSAA:
      return "MSAA";
  }
}

// Format used for MSAA renderbuffer storage.  kBest maps to
// msaa_shim::GetBestRenderbufferFormat whereas kRGB8 maps to GL_RGB8.  Passing
// an unsupported format will cause a non-MSAA buffer to be generated.
// Parameter is ignored for non-MSAA buffers.
enum class RenderTargetFormat { kBest = 0, kRGB8 = 1 };

// Provides a frame buffer to draw on, and has utilities to copy (blit) from
// this drawing surface to another surface.
class RenderTarget {
 public:
  explicit RenderTarget(std::shared_ptr<GLResourceManager> gl_resources);

  // filter is used for GL_TEXTURE_MIN_FILTER / GL_TEXTURE_MAG_FILTER
  // filter is not respected for AntialiasingStrategy::kMSAA
  // internal_format is only used for AntialiasingStrategy::kMSAA
  RenderTarget(std::shared_ptr<GLResourceManager> gl_resources,
               AntialiasingStrategy aastrategy, TextureMapping filter,
               RenderTargetFormat internal_format = RenderTargetFormat::kBest);

  // Disallow copy and assign.
  RenderTarget(const RenderTarget&) = delete;
  RenderTarget& operator=(const RenderTarget&) = delete;

  ~RenderTarget();
  void Resize(glm::ivec2 size);
  glm::ivec2 GetSize() const { return size_; }
  Rect Bounds() const { return Rect({0, 0}, size_); }

  // Binds the buffer as the current drawing surface.
  void Bind() const;

  // Clears the buffer, setting every pixel to the given color. Note that this
  // also binds the buffer in the process.
  // Warning: This is a not full reset of the render target, i.e. it does not
  // result in the same state as a newly-created instance.
  void Clear(glm::vec4 color = {0, 0, 0, 0});

  // Takes the image from buffer_source, and draws it at world_dest on the bound
  // surface. buffer_source is assumed to lie within the render target's
  // Bounds().
  // Warning: Draw is not currently allowed for MSAA targets; you must instead
  // Blit() to another non-MSAA RenderTarget.
  void Draw(const Camera& cam, const blit_attrs::BlitAttrs& attrs,
            const RotRect& buffer_source, const RotRect& world_dest) const;

  // This convenience function draws the entire render target such that it
  // covers the camera's visible window.
  void Draw(const Camera& cam, const blit_attrs::BlitAttrs& attrs) const {
    Draw(cam, attrs, RotRect(Bounds()), cam.WorldRotRect());
  }

  // Copies this render target into the destination RenderTarget.
  // For MSAA RenderTargets, the destination should be non-MSAA and have
  // the same size.
  //
  // Can optionally copy only an axis-aligned rect of the target, the
  // coordinates must match in both render targets, and the above constraints
  // still apply.  The rect is truncated to integer values in the pixel buffer
  // size of the backing FBO.
  void Blit(RenderTarget* destination,
            absl::optional<Rect> area = absl::nullopt) const;
  std::string ToString() const;

  bool msaa() const;

  // Transfers out to a Texture for further usage without allowing any further
  // drawing to that texture. This releases the associated fbo and rbo and
  // resets this render_target back to the default state.
  void TransferTexture(Texture* out);

  // Reads the pixels and writes them out to pixel_buffer_out, resizing the
  // vector if necessary. The type T is for convenience, the actual data written
  // will be tightly packed uint32_t in the format R8G8B8A8.
  //
  // It is assumed that the number of bytes of pixels will exactly fit into an
  // integer number of type T, i.e. the following holds true:
  // width * height * sizeof(uint32_t) % sizeof(T) == 0
  //
  // Returns a bool to match the interface of Texture::GetPixels()
  template <typename T>
  bool GetPixels(std::vector<T>* pixel_buffer_out) const {
    auto num_pixels = size_.x * size_.y;
    EXPECT(num_pixels > 0);

    // The number of bytes needed to store all the image pixel data (we will
    // read the image data in the format RGBA_8888, hence 32 bits per pixel.
    size_t num_bytes = num_pixels * sizeof(uint32_t);

    // It is assumed that the bytes will fix exactly into an integer number of
    // "T"s.
    EXPECT(num_bytes % sizeof(T) == 0);
    pixel_buffer_out->resize(num_bytes / sizeof(T));
    CaptureRawData(pixel_buffer_out->data());
    return true;
  }

  bool GetPixels(GPUPixels* pixels) const;

  GLuint GetFboId() const { return fbo_; }

 private:
  void CaptureRawData(void* pixel_buffer_out) const;

  std::shared_ptr<GLResourceManager> gl_resources_;
  TexturedQuadRenderer renderer_;
  void DeleteBuffers();
  void GenBuffers(glm::ivec2 size);
  glm::ivec2 size_{0, 0};
  Camera cam_;
  GLuint fbo_, rbo_;
  Texture tex_;
  AntialiasingStrategy aastrategy_;
  TextureMapping filter_;
  RenderTargetFormat internal_format_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_BASEGL_RENDER_TARGET_H_
