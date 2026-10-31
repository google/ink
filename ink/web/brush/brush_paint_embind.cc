// Copyright 2026 Google LLC
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

#include <emscripten/bind.h>

#include <string>
#include <utility>
#include <variant>

#include "ink/brush/brush_paint.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/vec.h"

namespace ink::web {
namespace {

EMSCRIPTEN_BINDINGS(ink_brush_paint_bindings) {
  emscripten::enum_<BrushPaint::TextureOrigin>("TextureOrigin")
      .value("STROKE_SPACE_ORIGIN",
             BrushPaint::TextureOrigin::kStrokeSpaceOrigin)
      .value("FIRST_STROKE_INPUT", BrushPaint::TextureOrigin::kFirstStrokeInput)
      .value("LAST_STROKE_INPUT", BrushPaint::TextureOrigin::kLastStrokeInput);

  emscripten::enum_<BrushPaint::TextureSizeUnit>("TextureSizeUnit")
      .value("BRUSH_SIZE", BrushPaint::TextureSizeUnit::kBrushSize)
      .value("STROKE_COORDINATES",
             BrushPaint::TextureSizeUnit::kStrokeCoordinates);

  emscripten::enum_<BrushPaint::TextureWrap>("TextureWrap")
      .value("REPEAT", BrushPaint::TextureWrap::kRepeat)
      .value("MIRROR", BrushPaint::TextureWrap::kMirror)
      .value("CLAMP", BrushPaint::TextureWrap::kClamp);

  emscripten::enum_<BrushPaint::BlendMode>("BlendMode")
      .value("MODULATE", BrushPaint::BlendMode::kModulate)
      .value("DST_IN", BrushPaint::BlendMode::kDstIn)
      .value("DST_OUT", BrushPaint::BlendMode::kDstOut)
      .value("SRC_ATOP", BrushPaint::BlendMode::kSrcAtop)
      .value("SRC_IN", BrushPaint::BlendMode::kSrcIn)
      .value("SRC_OVER", BrushPaint::BlendMode::kSrcOver)
      .value("DST_OVER", BrushPaint::BlendMode::kDstOver)
      .value("SRC", BrushPaint::BlendMode::kSrc)
      .value("DST", BrushPaint::BlendMode::kDst)
      .value("SRC_OUT", BrushPaint::BlendMode::kSrcOut)
      .value("DST_ATOP", BrushPaint::BlendMode::kDstAtop)
      .value("XOR", BrushPaint::BlendMode::kXor);

  emscripten::enum_<BrushPaint::SelfOverlap>("SelfOverlap")
      .value("ANY", BrushPaint::SelfOverlap::kAny)
      .value("ACCUMULATE", BrushPaint::SelfOverlap::kAccumulate)
      .value("DISCARD", BrushPaint::SelfOverlap::kDiscard);

  emscripten::class_<BrushPaint::TilingTexture>("TilingTexture")
      .constructor<>()
      .constructor(+[](std::string client_texture_id,
                       BrushPaint::TextureOrigin origin,
                       BrushPaint::TextureSizeUnit size_unit,
                       BrushPaint::TextureWrap wrap_x,
                       BrushPaint::TextureWrap wrap_y, Vec size, Vec offset,
                       Angle rotation, BrushPaint::BlendMode blend_mode) {
        return BrushPaint::TilingTexture{
            .client_texture_id = std::move(client_texture_id),
            .origin = origin,
            .size_unit = size_unit,
            .wrap_x = wrap_x,
            .wrap_y = wrap_y,
            .size = size,
            .offset = offset,
            .rotation = rotation,
            .blend_mode = blend_mode,
        };
      })
      .property("clientTextureId",
                &BrushPaint::TilingTexture::client_texture_id)
      .property("origin", &BrushPaint::TilingTexture::origin)
      .property("sizeUnit", &BrushPaint::TilingTexture::size_unit)
      .property("wrapX", &BrushPaint::TilingTexture::wrap_x)
      .property("wrapY", &BrushPaint::TilingTexture::wrap_y)
      .property("size", &BrushPaint::TilingTexture::size)
      .property("offset", &BrushPaint::TilingTexture::offset)
      .property("rotation", &BrushPaint::TilingTexture::rotation)
      .property("blendMode", &BrushPaint::TilingTexture::blend_mode);

  emscripten::class_<BrushPaint>("BrushPaint")
      .constructor<>()
      .class_function(
          "withTilingTexture",
          +[](const BrushPaint::TilingTexture& layer,
              BrushPaint::SelfOverlap self_overlap) {
            return BrushPaint{
                .texture_layers = {layer},
                .self_overlap = self_overlap,
            };
          })
      .function(
          "addTilingTextureLayer",
          +[](BrushPaint& paint, const BrushPaint::TilingTexture& layer) {
            paint.texture_layers.push_back(layer);
          })
      .function(
          "getTextureLayerCount",
          +[](const BrushPaint& paint) {
            return static_cast<int>(paint.texture_layers.size());
          })
      .function(
          "getTilingTextureLayer",
          +[](const BrushPaint& paint, int index) {
            return std::get<BrushPaint::TilingTexture>(
                paint.texture_layers.at(index));
          })
      .property("selfOverlap", &BrushPaint::self_overlap);
}

}  // namespace
}  // namespace ink::web
