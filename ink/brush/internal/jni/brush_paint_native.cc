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

#include "ink/brush/internal/jni/brush_paint_native.h"

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/color_function.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/color/internal/jni/color_native_helper.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/internal/jni/mesh_format_native_helper.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/vec.h"

namespace {

using ::ink::Angle;
using ::ink::BrushPaint;
using ::ink::Color;
using ::ink::ColorFunction;
using ::ink::ColorSpace;
using ::ink::MeshFormat;
using ::ink::Vec;
using ::ink::brush_internal::AddAttributeIdsRequiredByPaint;
using ::ink::brush_internal::ValidateBrushPaint;
using ::ink::brush_internal::ValidateBrushPaintTextureLayer;
using ::ink::native::CastToBrushPaint;
using ::ink::native::CastToColorFunction;
using ::ink::native::CastToMeshFormat;
using ::ink::native::CastToTextureLayer;
using ::ink::native::ComputeColorLong;
using ::ink::native::DeleteNativeBrushPaint;
using ::ink::native::DeleteNativeColorFunction;
using ::ink::native::DeleteNativeTextureLayer;
using ::ink::native::NewNativeBrushPaint;
using ::ink::native::NewNativeColorFunction;
using ::ink::native::NewNativeTextureLayer;

int64_t ValidateAndHoistColorFunctionOrThrow(
    ColorFunction::Parameters parameters, void* jni_env_pass_through,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  ColorFunction color_function{.parameters = std::move(parameters)};
  if (absl::Status status =
          ink::brush_internal::ValidateColorFunction(color_function);
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return 0;
  }
  return NewNativeColorFunction(std::move(color_function));
}

}  // namespace

extern "C" {

int64_t BrushPaintNative_create(
    void* jni_env_pass_through, const int64_t* texture_layer_native_pointers,
    int num_texture_layers, const int64_t* color_function_native_pointers,
    int num_color_functions, int self_overlap_int,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  std::vector<BrushPaint::TextureLayer> texture_layers;
  texture_layers.reserve(num_texture_layers);
  for (int i = 0; i < num_texture_layers; ++i) {
    texture_layers.push_back(
        CastToTextureLayer(texture_layer_native_pointers[i]));
  }

  std::vector<ColorFunction> color_functions;
  color_functions.reserve(num_color_functions);
  for (int i = 0; i < num_color_functions; ++i) {
    color_functions.push_back(
        CastToColorFunction(color_function_native_pointers[i]));
  }

  BrushPaint brush_paint{
      .texture_layers = std::move(texture_layers),
      .color_functions = std::move(color_functions),
      .self_overlap = static_cast<BrushPaint::SelfOverlap>(self_overlap_int)};
  if (absl::Status status = ValidateBrushPaint(brush_paint); !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return 0;
  }
  return NewNativeBrushPaint(std::move(brush_paint));
}

void BrushPaintNative_free(int64_t native_ptr) {
  DeleteNativeBrushPaint(native_ptr);
}

int BrushPaintNative_getTextureLayerCount(int64_t native_ptr) {
  return CastToBrushPaint(native_ptr).texture_layers.size();
}

int BrushPaintNative_getTextureLayerMappingInt(int64_t native_ptr, int index) {
  return CastToBrushPaint(native_ptr).texture_layers[index].index();
}

int64_t BrushPaintNative_newCopyOfTextureLayer(int64_t native_ptr, int index) {
  return NewNativeTextureLayer(
      CastToBrushPaint(native_ptr).texture_layers[index]);
}

int BrushPaintNative_getColorFunctionCount(int64_t native_ptr) {
  return CastToBrushPaint(native_ptr).color_functions.size();
}

int BrushPaintNative_getColorFunctionParametersTypeInt(int64_t native_ptr,
                                                       int index) {
  return CastToBrushPaint(native_ptr).color_functions[index].parameters.index();
}

int64_t BrushPaintNative_newCopyOfColorFunction(int64_t native_ptr, int index) {
  return NewNativeColorFunction(
      CastToBrushPaint(native_ptr).color_functions[index]);
}

int BrushPaintNative_getSelfOverlapInt(int64_t native_ptr) {
  return static_cast<int>(CastToBrushPaint(native_ptr).self_overlap);
}

bool BrushPaintNative_isCompatibleWithMeshFormat(
    int64_t native_ptr, int64_t mesh_format_native_ptr) {
  absl::flat_hash_set<MeshFormat::AttributeId> required_attribute_ids;
  AddAttributeIdsRequiredByPaint(CastToBrushPaint(native_ptr),
                                 required_attribute_ids);

  absl::Span<const MeshFormat::Attribute> mesh_attributes =
      CastToMeshFormat(mesh_format_native_ptr).Attributes();
  for (const MeshFormat::Attribute& attr : mesh_attributes) {
    required_attribute_ids.erase(attr.id);
  }
  return required_attribute_ids.empty();
}

// ************ Native C-API of BrushPaint TextureLayer ************

int64_t TilingTextureNative_create(
    void* jni_env_pass_through, const char* client_texture_id, float size_x,
    float size_y, float offset_x, float offset_y, float rotation_degrees,
    int size_unit, int origin, int wrap_x, int wrap_y, int blend_mode,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  BrushPaint::TextureLayer texture_layer = {BrushPaint::TilingTexture{
      .client_texture_id = client_texture_id,
      .origin = static_cast<BrushPaint::TextureOrigin>(origin),
      .size_unit = static_cast<BrushPaint::TextureSizeUnit>(size_unit),
      .wrap_x = static_cast<BrushPaint::TextureWrap>(wrap_x),
      .wrap_y = static_cast<BrushPaint::TextureWrap>(wrap_y),
      .size = Vec{size_x, size_y},
      .offset = Vec{offset_x, offset_y},
      .rotation = Angle::Degrees(rotation_degrees),
      .blend_mode = static_cast<BrushPaint::BlendMode>(blend_mode),
  }};
  if (absl::Status status = ValidateBrushPaintTextureLayer(texture_layer);
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return 0;
  }
  return NewNativeTextureLayer(std::move(texture_layer));
}

int64_t StampingTextureNative_create(
    void* jni_env_pass_through, const char* client_texture_id,
    int animation_frames, int animation_rows, int animation_columns,
    int64_t animation_duration_millis, int blend_mode,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  BrushPaint::TextureLayer texture_layer = {BrushPaint::StampingTexture{
      .client_texture_id = client_texture_id,
      .animation_frames = animation_frames,
      .animation_rows = animation_rows,
      .animation_columns = animation_columns,
      .animation_duration = absl::Milliseconds(animation_duration_millis),
      .blend_mode = static_cast<BrushPaint::BlendMode>(blend_mode),
  }};
  if (absl::Status status = ValidateBrushPaintTextureLayer(texture_layer);
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return 0;
  }
  return NewNativeTextureLayer(std::move(texture_layer));
}

void TextureLayerNative_free(int64_t native_ptr) {
  DeleteNativeTextureLayer(native_ptr);
}

const char* TilingTextureNative_getClientTextureId(int64_t native_ptr) {
  return std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
      .client_texture_id.c_str();
}

float TilingTextureNative_getSizeX(int64_t native_ptr) {
  return std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
      .size.x;
}

float TilingTextureNative_getSizeY(int64_t native_ptr) {
  return std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
      .size.y;
}

float TilingTextureNative_getOffsetX(int64_t native_ptr) {
  return std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
      .offset.x;
}

float TilingTextureNative_getOffsetY(int64_t native_ptr) {
  return std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
      .offset.y;
}

float TilingTextureNative_getRotationDegrees(int64_t native_ptr) {
  return std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
      .rotation.ValueInDegrees();
}

const char* StampingTextureNative_getClientTextureId(int64_t native_ptr) {
  return std::get<BrushPaint::StampingTexture>(CastToTextureLayer(native_ptr))
      .client_texture_id.c_str();
}

int StampingTextureNative_getAnimationFrames(int64_t native_ptr) {
  return std::get<BrushPaint::StampingTexture>(CastToTextureLayer(native_ptr))
      .animation_frames;
}

int StampingTextureNative_getAnimationRows(int64_t native_ptr) {
  return std::get<BrushPaint::StampingTexture>(CastToTextureLayer(native_ptr))
      .animation_rows;
}

int StampingTextureNative_getAnimationColumns(int64_t native_ptr) {
  return std::get<BrushPaint::StampingTexture>(CastToTextureLayer(native_ptr))
      .animation_columns;
}

int64_t StampingTextureNative_getAnimationDurationMillis(int64_t native_ptr) {
  return absl::ToInt64Milliseconds(
      std::get<BrushPaint::StampingTexture>(CastToTextureLayer(native_ptr))
          .animation_duration);
}

int TilingTextureNative_getSizeUnitInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
          .size_unit);
}

int TilingTextureNative_getOriginInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
          .origin);
}

int TextureLayerNative_getMappingInt(int64_t native_ptr) {
  return CastToTextureLayer(native_ptr).index();
}

int TilingTextureNative_getWrapXInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
          .wrap_x);
}

int TilingTextureNative_getWrapYInt(int64_t native_ptr) {
  return static_cast<int>(
      std::get<BrushPaint::TilingTexture>(CastToTextureLayer(native_ptr))
          .wrap_y);
}

int TextureLayerNative_getBlendModeInt(int64_t native_ptr) {
  return static_cast<int>(
      std::visit([](const auto& layer) { return layer.blend_mode; },
                 CastToTextureLayer(native_ptr)));
}

// ************ Native C-API of BrushPaint ColorFunction ************

int64_t ColorFunctionNative_createOpacityMultiplier(
    void* jni_env_pass_through, float multiplier,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  return ValidateAndHoistColorFunctionOrThrow(
      ColorFunction::OpacityMultiplier{.multiplier = multiplier},
      jni_env_pass_through, throw_from_status_callback);
}

int64_t ColorFunctionNative_createHueOffset(
    void* jni_env_pass_through, float offsetDegrees,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  return ValidateAndHoistColorFunctionOrThrow(
      ColorFunction::HueOffset{.offset = Angle::Degrees(offsetDegrees)},
      jni_env_pass_through, throw_from_status_callback);
}

int64_t ColorFunctionNative_createSaturationMultiplier(
    void* jni_env_pass_through, float multiplier,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  return ValidateAndHoistColorFunctionOrThrow(
      ColorFunction::SaturationMultiplier{.multiplier = multiplier},
      jni_env_pass_through, throw_from_status_callback);
}

int64_t ColorFunctionNative_createLuminosityOffset(
    void* jni_env_pass_through, float offset,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  return ValidateAndHoistColorFunctionOrThrow(
      ColorFunction::LuminosityOffset{.offset = offset}, jni_env_pass_through,
      throw_from_status_callback);
}

int64_t ColorFunctionNative_createReplaceColor(
    void* jni_env_pass_through, float color_red, float color_green,
    float color_blue, float color_alpha, int color_space_id,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  return ValidateAndHoistColorFunctionOrThrow(
      ColorFunction::ReplaceColor{
          .color = Color::FromFloat(color_red, color_green, color_blue,
                                    color_alpha, Color::Format::kGammaEncoded,
                                    static_cast<ColorSpace>(color_space_id))},
      jni_env_pass_through, throw_from_status_callback);
}

void ColorFunctionNative_free(int64_t native_ptr) {
  DeleteNativeColorFunction(native_ptr);
}

float ColorFunctionNative_getOpacityMultiplier(int64_t native_ptr) {
  return std::get<ColorFunction::OpacityMultiplier>(
             CastToColorFunction(native_ptr).parameters)
      .multiplier;
}

float ColorFunctionNative_getHueOffsetDegrees(int64_t native_ptr) {
  return std::get<ColorFunction::HueOffset>(
             CastToColorFunction(native_ptr).parameters)
      .offset.ValueInDegrees();
}

float ColorFunctionNative_getSaturationMultiplier(int64_t native_ptr) {
  return std::get<ColorFunction::SaturationMultiplier>(
             CastToColorFunction(native_ptr).parameters)
      .multiplier;
}

float ColorFunctionNative_getLuminosityOffset(int64_t native_ptr) {
  return std::get<ColorFunction::LuminosityOffset>(
             CastToColorFunction(native_ptr).parameters)
      .offset;
}

int64_t ColorFunctionNative_computeReplaceColorLong(
    void* jni_env_pass_through, int64_t native_ptr,
    int64_t (*compose_color_long_from_components_callback)(void*, int, float,
                                                           float, float,
                                                           float)) {
  return ComputeColorLong(jni_env_pass_through,
                          std::get<ColorFunction::ReplaceColor>(
                              CastToColorFunction(native_ptr).parameters)
                              .color,
                          compose_color_long_from_components_callback);
}

int64_t ColorFunctionNative_computeTransformedColorLong(
    void* jni_env_pass_through, int64_t native_ptr, float color_red,
    float color_green, float color_blue, float color_alpha, int color_space_id,
    int64_t (*compose_color_long_from_components_callback)(void*, int, float,
                                                           float, float,
                                                           float)) {
  const Color input_color = Color::FromFloat(
      color_red, color_green, color_blue, color_alpha,
      Color::Format::kGammaEncoded, static_cast<ColorSpace>(color_space_id));
  const Color output_color = CastToColorFunction(native_ptr)(input_color);
  return ComputeColorLong(jni_env_pass_through, output_color,
                          compose_color_long_from_components_callback);
}

}  // extern "C"
