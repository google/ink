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

#include "ink/brush/internal/jni/brush_coat_native.h"

#include <cstdint>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/types/span.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/geometry/internal/jni/mesh_format_native_helper.h"
#include "ink/geometry/mesh_format.h"

using ::ink::BrushCoat;
using ::ink::BrushPaint;
using ::ink::MeshFormat;
using ::ink::brush_internal::AddAttributeIdsRequiredByCoat;
using ::ink::brush_internal::CalculateMinimumRequiredVersion;
using ::ink::native::CastToBrushCoat;
using ::ink::native::CastToBrushPaint;
using ::ink::native::CastToBrushTip;
using ::ink::native::CastToMeshFormat;
using ::ink::native::DeleteNativeBrushCoat;
using ::ink::native::NewNativeBrushCoat;
using ::ink::native::NewNativeBrushPaint;
using ::ink::native::NewNativeBrushTip;

extern "C" {

int64_t BrushCoatNative_create(int64_t tip_native_pointer,
                               const int64_t* paint_preferences_native_pointers,
                               int num_paint_preferences) {
  absl::InlinedVector<BrushPaint, 1> paint_preferences;
  ABSL_CHECK_GT(num_paint_preferences, 0);
  paint_preferences.reserve(num_paint_preferences);
  for (int i = 0; i < num_paint_preferences; ++i) {
    int64_t paint_native_pointer = paint_preferences_native_pointers[i];
    paint_preferences.push_back(CastToBrushPaint(paint_native_pointer));
  }
  return NewNativeBrushCoat(BrushCoat{
      .tip = CastToBrushTip(tip_native_pointer),
      .paint_preferences = std::move(paint_preferences),
  });
}

void BrushCoatNative_free(int64_t native_pointer) {
  DeleteNativeBrushCoat(native_pointer);
}

bool BrushCoatNative_isCompatibleWithMeshFormat(
    int64_t native_pointer, int64_t mesh_format_native_pointer) {
  // Gather all the attributes that are required by the brush coat.
  absl::flat_hash_set<MeshFormat::AttributeId> required_attribute_ids;
  AddAttributeIdsRequiredByCoat(CastToBrushCoat(native_pointer),
                                required_attribute_ids);

  // Check if all required attributes are present in the mesh format.
  absl::Span<const MeshFormat::Attribute> mesh_attributes =
      CastToMeshFormat(mesh_format_native_pointer).Attributes();
  for (const MeshFormat::Attribute& attr : mesh_attributes) {
    required_attribute_ids.erase(attr.id);
  }
  return required_attribute_ids.empty();
}

int64_t BrushCoatNative_newCopyOfBrushTip(int64_t native_pointer) {
  return NewNativeBrushTip(CastToBrushCoat(native_pointer).tip);
}

int BrushCoatNative_getBrushPaintPreferencesCount(int64_t native_pointer) {
  return CastToBrushCoat(native_pointer).paint_preferences.size();
}

int64_t BrushCoatNative_newCopyOfBrushPaintPreference(int64_t native_pointer,
                                                      int index) {
  return NewNativeBrushPaint(
      CastToBrushCoat(native_pointer).paint_preferences[index]);
}

int BrushCoatNative_calculateMinimumRequiredVersion(int64_t native_pointer) {
  return CalculateMinimumRequiredVersion(CastToBrushCoat(native_pointer))
      .value();
}

}  // extern "C"
