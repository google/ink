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

#include "ink/engine/rendering/gl_managers/nine_patch_info.h"

#include <algorithm>
#include <cstdint>

#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"

namespace ink {

using util::Normalize;

using BorderId = NinePatchInfo::BorderId;

namespace {
void NormalizePairIntoArray4(float* array, std::pair<size_t, size_t> pt,
                             int n_texels_on_axis) {
  double n_texels = static_cast<double>(n_texels_on_axis);

  // b/24133013, we should get rid of these half pixel corrections and only load
  // the pixels that we need.
  //
  // (0,3 critical points are setup to take away the 1px border on each side)
  array[0] = Normalize(0.0, n_texels, 1.5);
  array[1] = Normalize(0.0, n_texels, static_cast<double>(pt.first + 0.5));
  array[2] = Normalize(0.0, n_texels, static_cast<double>(pt.second + 0.5));
  array[3] = Normalize(0.0, n_texels, n_texels - 1.5);
}
}  // namespace

NinePatchInfo::NinePatchInfo() : is_nine_patch(false) {
  std::fill(x_stretch_critical_points, x_stretch_critical_points + 4, 0.0);
  std::fill(y_stretch_critical_points, y_stretch_critical_points + 4, 0.0);
  std::fill(x_fill_critical_points, x_fill_critical_points + 4, 0.0);
  std::fill(y_fill_critical_points, y_fill_critical_points + 4, 0.0);
}

// helper to read border pixels, returning the critical points
bool NinePatchInfo::ReadBorderStartStop(const ClientBitmap& img,
                                        BorderId which_border,
                                        std::pair<size_t, size_t>* result) {
  ASSERT(result);
  size_in_px = glm::ivec2(img.sizeInPx().width, img.sizeInPx().height);
  auto format = img.format();
  *result = std::pair<size_t, size_t>(0, 0);

  size_t bytes_per_texel = img.bytesPerTexel();
  size_t n_texels_x = static_cast<size_t>(size_in_px.x);
  size_t n_texels_y = static_cast<size_t>(size_in_px.y);
  unsigned char const* data =
      static_cast<unsigned char const*>(img.imageByteData());
  unsigned char const* data_end =
      data + (n_texels_x * n_texels_y * bytes_per_texel);
  size_t limit =
      (which_border == BorderId::LEFT || which_border == BorderId::RIGHT)
          ? n_texels_y
          : n_texels_x;

  size_t start_texel = 0;
  size_t end_texel = 0;
  bool found_start = false;
  bool found_end = false;

  for (size_t i = 0; i < limit; i++) {
    size_t byte_offset = 0;
    switch (which_border) {
      case BorderId::LEFT:
        byte_offset = i * bytes_per_texel * n_texels_x;
        break;
      case BorderId::TOP:
        byte_offset = i * bytes_per_texel;
        break;
      case BorderId::RIGHT:
        // pointer to the start of the next row minus 1
        byte_offset =
            ((i + 1) * bytes_per_texel * n_texels_x) - (1 * bytes_per_texel);
        break;
      case BorderId::BOTTOM:
        // pointer to the last row plus location on the last row
        byte_offset = bytes_per_texel * n_texels_x * (n_texels_y - 1) +
                      (i * bytes_per_texel);
        break;
    }
    if (data + byte_offset > data_end || data + byte_offset < data) {
      ASSERT(false);
      return false;
    }

    uint32_t current_texel_rgba8888;
    if (!expandTexelToRGBA8888(format, data + byte_offset, data_end,
                               &current_texel_rgba8888)) {
      SLOG(SLOG_ERROR, "could not interpret png data. (bad format?)");
      return false;
    }

    glm::vec4 current_color = UintToVec4RGBA(current_texel_rgba8888);
    bool on = (current_color == glm::vec4(0, 0, 0, 1));
    if (!found_start && on) {
      found_start = true;
      start_texel = i;
    } else if (found_start && !found_end && !on) {
      found_end = true;
      end_texel = i;
    }
#if INK_DEBUG
    if (found_start && found_end) {
      ASSERT(!on);
    }
#endif  // INK_DEBUG
  }

  if (!found_start || !found_end) {
    SLOG(SLOG_ERROR, "could not interpret ($0) as a nine patch!", img);
    ASSERT(false);

    return false;
  }
  *result = std::make_pair(start_texel, end_texel);
  return true;
}

NinePatchInfo::NinePatchInfo(const ClientBitmap& img) : NinePatchInfo() {
  glm::ivec2 size_in_px(img.sizeInPx().width, img.sizeInPx().height);
  if (size_in_px.x <= 0 || size_in_px.y <= 0 || img.bytesPerTexel() == 0 ||
      !img.imageByteData()) {
    SLOG(SLOG_ERROR, "could not interpret ($0) as a nine patch!", img);
    ASSERT(false);
    return;
  }
  std::pair<size_t, size_t> left_start_stop;
  std::pair<size_t, size_t> top_start_stop;
  std::pair<size_t, size_t> right_start_stop;
  std::pair<size_t, size_t> bottom_start_stop;

  // Read critical points
  bool read_left = ReadBorderStartStop(img, BorderId::LEFT, &left_start_stop);
  bool read_top = ReadBorderStartStop(img, BorderId::TOP, &top_start_stop);
  bool read_right =
      ReadBorderStartStop(img, BorderId::RIGHT, &right_start_stop);
  bool read_bottom =
      ReadBorderStartStop(img, BorderId::BOTTOM, &bottom_start_stop);

  if (read_left && read_top && read_right && read_bottom) {
    NormalizePairIntoArray4(y_stretch_critical_points, left_start_stop,
                            size_in_px.y);
    NormalizePairIntoArray4(x_stretch_critical_points, top_start_stop,
                            size_in_px.x);
    NormalizePairIntoArray4(y_fill_critical_points, right_start_stop,
                            size_in_px.y);
    NormalizePairIntoArray4(x_fill_critical_points, bottom_start_stop,
                            size_in_px.x);

    // Calc uv to texel transform
    Rect center_rect_uv(
        x_stretch_critical_points[1], y_stretch_critical_points[1],
        x_stretch_critical_points[2], y_stretch_critical_points[2]);
    Rect center_rect_texel(top_start_stop.first, left_start_stop.first,
                           top_start_stop.second, left_start_stop.second);
    uv_to_texel = center_rect_uv.CalcTransformTo(center_rect_texel);
    is_nine_patch = true;
  }
}
}  // namespace ink
