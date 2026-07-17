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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_COAT_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_COAT_NATIVE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BrushCoatNative_create(int64_t tip_native_pointer,
                               const int64_t* paint_preferences_native_pointers,
                               int num_paint_preferences);

void BrushCoatNative_free(int64_t native_pointer);

bool BrushCoatNative_isCompatibleWithMeshFormat(
    int64_t native_pointer, int64_t mesh_format_native_pointer);

int64_t BrushCoatNative_newCopyOfBrushTip(int64_t native_pointer);

int BrushCoatNative_getBrushPaintPreferencesCount(int64_t native_pointer);

int64_t BrushCoatNative_newCopyOfBrushPaintPreference(int64_t native_pointer,
                                                      int index);

int BrushCoatNative_calculateMinimumRequiredVersion(int64_t native_pointer);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_COAT_NATIVE_H_
