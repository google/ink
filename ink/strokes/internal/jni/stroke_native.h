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

#ifndef INK_STROKES_INTERNAL_JNI_STROKE_NATIVE_H_
#define INK_STROKES_INTERNAL_JNI_STROKE_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t StrokeNative_createWithBrushAndInputs(int64_t brush_native_pointer,
                                              int64_t inputs_native_pointer);

int64_t StrokeNative_createWithBrushInputsAndShape(
    int64_t brush_native_pointer, int64_t inputs_native_pointer,
    int64_t partitioned_mesh_native_pointer);

// Make a heap-allocated shallow (doesn't replicate all the individual input
// points) copy of the `StrokeInputBatch` belonging to this `Stroke`. Return
// the raw pointer to this copy, so that it can be wrapped by a JVM
// `StrokeInputBatch`, which is responsible for freeing the copy when it is
// garbage collected and finalized.
int64_t StrokeNative_newShallowCopyOfInputs(int64_t native_pointer_to_stroke);

// Make a heap-allocated shallow (doesn't replicate all the individual meshes)
// copy of the `PartitionedMesh` belonging to this `Stroke`. Return the raw
// pointer to this copy, so that it can be wrapped by a JVM `PartitionedMesh`,
// which is responsible for freeing the copy when it is garbage collected and
// finalized.
int64_t StrokeNative_newShallowCopyOfShape(int64_t native_pointer_to_stroke);

void StrokeNative_free(int64_t native_pointer_to_stroke);

// Returns a pointer to hold the result of a partial erase before hand-off
// of the individual strokes. Uses `unique_ptr` for the handoff to minimize
// copying.
int64_t MultipleStrokesNative_createWithPartialErase(
    int64_t target_stroke_ptr, int64_t eraser_shape_ptr, float eraser_a,
    float eraser_b, float eraser_c, float eraser_d, float eraser_e,
    float eraser_f, float stroke_a, float stroke_b, float stroke_c,
    float stroke_d, float stroke_e, float stroke_f);

int32_t MultipleStrokesNative_getStrokeCount(int64_t native_pointer);

// Releases the pointer to i-th stroke in partial erase result, to be wrapped
// in a Kotlin `Stroke`.
int64_t MultipleStrokesNative_releaseStroke(int64_t native_pointer,
                                            int32_t index);

// Frees the remaining result of the partial erase.
void MultipleStrokesNative_free(int64_t native_pointer);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_STROKES_INTERNAL_JNI_STROKE_NATIVE_H_
