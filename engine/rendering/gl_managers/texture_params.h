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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_PARAMS_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_PARAMS_H_

#include <utility>

#include "ink/engine/gl.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/proto/sengine_portable_proto.pb.h"

enum class TextureWrap { ClampToEdge, MirroredRepeat, Repeat };

enum class TextureMapping { Nearest, Linear };

namespace ink {

template <>
inline std::string Str(const TextureWrap& tw) {
  switch (tw) {
    case TextureWrap::ClampToEdge:
      return "ClampToEdge";
    case TextureWrap::MirroredRepeat:
      return "MirroredRepeat";
    case TextureWrap::Repeat:
      return "Repeat";
  }
}

template <>
inline std::string Str(const TextureMapping& tm) {
  switch (tm) {
    case TextureMapping::Nearest:
      return "Nearest";
    case TextureMapping::Linear:
      return "Linear";
  }
}

GLint GlTextureWrap(TextureWrap texture_wrap);
GLint GlTextureFilter(TextureMapping texture_mapping, bool use_mipmap);

// See https://www.khronos.org/opengles/sdk/docs/man/xhtml/glTexImage2D.xml and
// https://www.khronos.org/registry/OpenGL-Refpages/es2.0/xhtml/glPixelStorei.xml
struct GlTextureFormatInfo {
  GLenum internal_format;
  GLenum format;
  GLenum type;
  int unpack_alignment;
  bool should_convert_to_rgba_8888;
};
GlTextureFormatInfo GlTextureFormat(ImageFormat format);

struct TextureParams {
  // These specify the wrap strategies used on the x- and y-axes, respectively.
  // See also glTexParameteri, GL_TEXTURE_WRAP_S (x-axis), and GL_TEXTURE_WRAP_T
  // (y-axis).
  TextureWrap wrap_x = TextureWrap::ClampToEdge;
  TextureWrap wrap_y = TextureWrap::ClampToEdge;

  // These specify the interpolation functions to use when mapping from pixels
  // to texels. The minifying filter is used when the area of the pixel is less
  // than one texel, and the magnifying filter is used when it is greater.
  // See also glTexParameteri, GL_TEXTURE_MIN_FILTER, and GL_TEXTURE_MAG_FILTER.
  TextureMapping minify_filter = TextureMapping::Linear;
  TextureMapping magnify_filter = TextureMapping::Linear;

  // This specifies whether mipmaps should be generated for the texture.
  // See also glGenerateMipmap.
  bool use_mipmap = false;

  // This specifies whether the texture should be used for transparency-based
  // hit-testing for image rectangle elements.
  bool use_for_hit_testing = false;

  // This specifies whether the texture is a scalable nine-patch image.
  bool is_nine_patch = false;

  /// This specifies whether this is a stand-in texture for a rejected texture
  /// URI.
  bool is_rejection = false;

  TextureParams() : TextureParams(proto::ImageInfo::DEFAULT) {}
  explicit TextureParams(proto::ImageInfo::AssetType asset_type);
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_TEXTURE_PARAMS_H_
