// Copyright 2024-2026 Google LLC
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

#include "ink/strokes/internal/jni/stroke_native.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/geometry/affine_transform.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native_helper.h"
#include "ink/strokes/internal/jni/stroke_input_batch_native_helper.h"
#include "ink/strokes/internal/jni/stroke_native_helper.h"
#include "ink/strokes/stroke.h"

namespace {

using ::ink::AffineTransform;
using ::ink::Stroke;
using ::ink::native::CastToBrush;
using ::ink::native::CastToPartitionedMesh;
using ::ink::native::CastToStroke;
using ::ink::native::CastToStrokeInputBatch;
using ::ink::native::DeleteNativeStroke;
using ::ink::native::NewNativePartitionedMesh;
using ::ink::native::NewNativeStroke;
using ::ink::native::NewNativeStrokeInputBatch;

using MultipleStrokes = std::vector<std::unique_ptr<Stroke>>;

inline int64_t NewNativeMultipleStrokes(MultipleStrokes&& strokes) {
  return reinterpret_cast<int64_t>(
      new MultipleStrokes(std::forward<MultipleStrokes>(strokes)));
}

inline MultipleStrokes& CastToMutableMultipleStrokes(int64_t native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0);
  return *reinterpret_cast<MultipleStrokes*>(native_pointer);
}

inline void DeleteMultipleStrokes(int64_t native_pointer) {
  ABSL_CHECK_NE(native_pointer, 0);
  delete reinterpret_cast<MultipleStrokes*>(native_pointer);
}

}  // namespace

extern "C" {

int64_t StrokeNative_createWithBrushAndInputs(int64_t brush_native_pointer,
                                              int64_t inputs_native_pointer) {
  return NewNativeStroke(Stroke(CastToBrush(brush_native_pointer),
                                CastToStrokeInputBatch(inputs_native_pointer)));
}

int64_t StrokeNative_createWithBrushInputsAndShape(
    int64_t brush_native_pointer, int64_t inputs_native_pointer,
    int64_t partitioned_mesh_native_pointer) {
  return NewNativeStroke(
      Stroke(CastToBrush(brush_native_pointer),
             CastToStrokeInputBatch(inputs_native_pointer),
             CastToPartitionedMesh(partitioned_mesh_native_pointer)));
}

int64_t StrokeNative_newShallowCopyOfInputs(int64_t native_pointer) {
  return NewNativeStrokeInputBatch(CastToStroke(native_pointer).GetInputs());
}

int64_t StrokeNative_newShallowCopyOfShape(int64_t native_pointer) {
  return NewNativePartitionedMesh(CastToStroke(native_pointer).GetShape());
}

void StrokeNative_free(int64_t native_pointer) {
  DeleteNativeStroke(native_pointer);
}

int64_t MultipleStrokesNative_createWithPartialErase(
    int64_t target_stroke_ptr, int64_t eraser_shape_ptr, float eraser_a,
    float eraser_b, float eraser_c, float eraser_d, float eraser_e,
    float eraser_f, float stroke_a, float stroke_b, float stroke_c,
    float stroke_d, float stroke_e, float stroke_f) {
  AffineTransform eraser_transform(eraser_a, eraser_b, eraser_c, eraser_d,
                                   eraser_e, eraser_f);
  AffineTransform stroke_transform(stroke_a, stroke_b, stroke_c, stroke_d,
                                   stroke_e, stroke_f);

  std::vector<Stroke> fragments =
      CastToStroke(target_stroke_ptr)
          .PartialErase(CastToPartitionedMesh(eraser_shape_ptr),
                        eraser_transform, stroke_transform);

  MultipleStrokes result;
  result.reserve(fragments.size());
  for (size_t i = 0; i < fragments.size(); ++i) {
    result.push_back(std::make_unique<Stroke>(std::move(fragments[i])));
  }
  return NewNativeMultipleStrokes(std::move(result));
}

int32_t MultipleStrokesNative_getStrokeCount(int64_t native_pointer) {
  return CastToMutableMultipleStrokes(native_pointer).size();
}

int64_t MultipleStrokesNative_releaseStroke(int64_t native_pointer,
                                            int32_t index) {
  return reinterpret_cast<int64_t>(
      CastToMutableMultipleStrokes(native_pointer)[index].release());
}

void MultipleStrokesNative_free(int64_t native_pointer) {
  DeleteMultipleStrokes(native_pointer);
}

}  // extern "C"
