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

#include "ink/engine/rendering/gl_managers/texture_params.h"

#include <cstdint>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"

namespace ink {

GLint GlTextureFilter(TextureMapping mapping, bool use_mipmap_filter) {
  switch (mapping) {
    case TextureMapping::Nearest:
      return use_mipmap_filter ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
    case TextureMapping::Linear:
      return use_mipmap_filter ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    default:
      ASSERT(false);
      SLOG(SLOG_ERROR, "Unrecognized texture mapping: $0",
           static_cast<int32_t>(mapping));
      return GL_LINEAR;
  }
}

GLint GlTextureWrap(TextureWrap texture_wrap) {
  switch (texture_wrap) {
    case TextureWrap::ClampToEdge:
      return GL_CLAMP_TO_EDGE;
    case TextureWrap::MirroredRepeat:
      return GL_MIRRORED_REPEAT;
    case TextureWrap::Repeat:
      return GL_REPEAT;
    default:
      ASSERT(false);
      SLOG(SLOG_ERROR, "Unrecognized texture wrap format: $0",
           static_cast<int32_t>(texture_wrap));
      return GL_CLAMP_TO_EDGE;
  }
}

GlTextureFormatInfo GlTextureFormat(ImageFormat format) {
  switch (format) {
    case ImageFormat::BITMAP_FORMAT_RGBA_8888:
      return GlTextureFormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 4};
    case ImageFormat::BITMAP_FORMAT_RGB_888:
      return GlTextureFormatInfo{GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 1};
    case ImageFormat::BITMAP_FORMAT_RGBA_4444:
      return GlTextureFormatInfo{GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4,
                                 2};
    case ImageFormat::BITMAP_FORMAT_RGB_565:
      return GlTextureFormatInfo{GL_RGB, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 2};
    case ImageFormat::BITMAP_FORMAT_A_8:
      return GlTextureFormatInfo{GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, 1};
    case ImageFormat::BITMAP_FORMAT_BGRA_8888:
#ifdef __APPLE__
      return GlTextureFormatInfo{GL_RGBA, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 4};
#endif
    default:
      RUNTIME_ERROR(
          "unknown imageformat $0 while attempting to convert to gl values",
          format);
  }
}

using proto::ImageInfo;
TextureParams::TextureParams(ImageInfo::AssetType asset_type) {
  if (asset_type == ImageInfo::BORDER) {
    minify_filter = TextureMapping::Nearest;
    magnify_filter = TextureMapping::Nearest;
    is_nine_patch = true;
  } else if (asset_type == ImageInfo::STICKER) {
    use_for_hit_testing = true;
  } else if (asset_type == ImageInfo::GRID) {
    wrap_x = TextureWrap::Repeat;
    wrap_y = TextureWrap::Repeat;
    magnify_filter = TextureMapping::Nearest;
    use_mipmap = true;
  } else if (asset_type == ImageInfo::TILED_TEXTURE) {
    wrap_x = TextureWrap::Repeat;
    wrap_y = TextureWrap::Repeat;
    magnify_filter = TextureMapping::Linear;
    use_mipmap = true;
  }
}

}  // namespace ink
