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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_PAINT_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_PAINT_NATIVE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BrushPaintNative_create(
    void* jni_env_pass_through, const int64_t* texture_layer_native_pointers,
    int num_texture_layers, const int64_t* color_function_native_pointers,
    int num_color_functions, int self_overlap_int,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

void BrushPaintNative_free(int64_t native_ptr);

int BrushPaintNative_getTextureLayerCount(int64_t native_ptr);

int BrushPaintNative_getTextureLayerMappingInt(int64_t native_ptr, int index);

int64_t BrushPaintNative_newCopyOfTextureLayer(int64_t native_ptr, int index);

int BrushPaintNative_getColorFunctionCount(int64_t native_ptr);

int BrushPaintNative_getColorFunctionParametersTypeInt(int64_t native_ptr,
                                                       int index);

int64_t BrushPaintNative_newCopyOfColorFunction(int64_t native_ptr, int index);

int BrushPaintNative_getSelfOverlapInt(int64_t native_ptr);

bool BrushPaintNative_isCompatibleWithMeshFormat(
    int64_t native_ptr, int64_t mesh_format_native_ptr);

// Native interface for BrushPaint.TextureLayer:

int64_t TilingTextureNative_create(
    void* jni_env_pass_through, const char* client_texture_id, float size_x,
    float size_y, float offset_x, float offset_y, float rotation_degrees,
    int size_unit, int origin, int wrap_x, int wrap_y, int blend_mode,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

int64_t StampingTextureNative_create(
    void* jni_env_pass_through, const char* client_texture_id,
    int animation_frames, int animation_rows, int animation_columns,
    int64_t animation_duration_millis, int blend_mode,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

void TextureLayerNative_free(int64_t native_ptr);

const char* TilingTextureNative_getClientTextureId(int64_t native_ptr);

float TilingTextureNative_getSizeX(int64_t native_ptr);

float TilingTextureNative_getSizeY(int64_t native_ptr);

float TilingTextureNative_getOffsetX(int64_t native_ptr);

float TilingTextureNative_getOffsetY(int64_t native_ptr);

float TilingTextureNative_getRotationDegrees(int64_t native_ptr);

const char* StampingTextureNative_getClientTextureId(int64_t native_ptr);

int StampingTextureNative_getAnimationFrames(int64_t native_ptr);

int StampingTextureNative_getAnimationRows(int64_t native_ptr);

int StampingTextureNative_getAnimationColumns(int64_t native_ptr);

int64_t StampingTextureNative_getAnimationDurationMillis(int64_t native_ptr);

int TilingTextureNative_getSizeUnitInt(int64_t native_ptr);

int TilingTextureNative_getOriginInt(int64_t native_ptr);

int TextureLayerNative_getMappingInt(int64_t native_ptr);

int TilingTextureNative_getWrapXInt(int64_t native_ptr);

int TilingTextureNative_getWrapYInt(int64_t native_ptr);

int TextureLayerNative_getBlendModeInt(int64_t native_ptr);

// Native interface for BrushPaint.ColorFunction:

int64_t ColorFunctionNative_createOpacityMultiplier(
    void* jni_env_pass_through, float multiplier,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

int64_t ColorFunctionNative_createHueOffset(
    void* jni_env_pass_through, float offsetDegrees,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

int64_t ColorFunctionNative_createSaturationMultiplier(
    void* jni_env_pass_through, float multiplier,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

int64_t ColorFunctionNative_createLuminosityOffset(
    void* jni_env_pass_through, float offset,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

int64_t ColorFunctionNative_createReplaceColor(
    void* jni_env_pass_through, float color_red, float color_green,
    float color_blue, float color_alpha, int color_space_id,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

void ColorFunctionNative_free(int64_t native_ptr);

float ColorFunctionNative_getOpacityMultiplier(int64_t native_ptr);

float ColorFunctionNative_getHueOffsetDegrees(int64_t native_ptr);

float ColorFunctionNative_getSaturationMultiplier(int64_t native_ptr);

float ColorFunctionNative_getLuminosityOffset(int64_t native_ptr);

int64_t ColorFunctionNative_computeReplaceColorLong(
    void* jni_env_pass_through, int64_t native_ptr,
    int64_t (*compose_color_long_from_components_callback)(void*, int, float,
                                                           float, float,
                                                           float));

int64_t ColorFunctionNative_computeTransformedColorLong(
    void* jni_env_pass_through, int64_t native_ptr, float color_red,
    float color_green, float color_blue, float color_alpha, int color_space_id,
    int64_t (*compose_color_long_from_components_callback)(void*, int, float,
                                                           float, float,
                                                           float));

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_PAINT_NATIVE_H_
