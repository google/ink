// Copyright 2024-2025 Google LLC
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

#include <jni.h>

#include "absl/log/absl_check.h"
#include "ink/brush/brush.h"
#include "ink/brush/internal/jni/brush_jni_helper.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/strokes/input/stroke_input_batch.h"
#include "ink/strokes/stroke.h"

namespace {

using ::ink::Brush;
using ::ink::PartitionedMesh;
using ::ink::Stroke;
using ::ink::StrokeInputBatch;
using ::ink::jni::CastToBrush;

// Cast the raw pointer to a **const** `Stroke` pointer. Note that `Stroke`
// is **immutable** in Kotlin/Java, so none of the operations here should
// modify a C++ `Stroke`. Instead, writes must happen in a copied `Stroke`
// object, and the raw pointer to that copied object should be returned to the
// Kotlin/Java layer to be wrapped in a new Kotlin `Stroke`.
const Stroke* GetStroke(jlong native_pointer_to_stroke) {
  ABSL_CHECK_NE(native_pointer_to_stroke, 0);
  return reinterpret_cast<Stroke*>(native_pointer_to_stroke);
}

}  // namespace

extern "C" {

JNI_METHOD(strokes, StrokeNative, jlong, createWithBrushAndInputs)
(JNIEnv* env, jobject object, jlong brush_native_pointer,
 jlong inputs_native_pointer) {
  auto batch = reinterpret_cast<StrokeInputBatch*>(inputs_native_pointer);
  const Brush& brush = CastToBrush(brush_native_pointer);
  return reinterpret_cast<jlong>(new Stroke(brush, *batch));
}

JNI_METHOD(strokes, StrokeNative, jlong, createWithBrushInputsAndShape)
(JNIEnv* env, jobject object, jlong brush_native_pointer,
 jlong inputs_native_pointer, jlong partitioned_mesh_native_pointer) {
  const Brush& brush = CastToBrush(brush_native_pointer);
  auto batch = reinterpret_cast<StrokeInputBatch*>(inputs_native_pointer);
  auto shape =
      reinterpret_cast<PartitionedMesh*>(partitioned_mesh_native_pointer);
  return reinterpret_cast<jlong>(new Stroke(brush, *batch, *shape));
}

// Make a heap-allocated shallow (doesn't replicate all the individual input
// points) copy of the `StrokeInputBatch` belonging to this `Stroke`. Return
// the raw pointer to this copy, so that it can be wrapped by a JVM
// `StrokeInputBatch`, which is responsible for freeing the copy when it is
// garbage collected and finalized.
JNI_METHOD(strokes, StrokeNative, jlong, newShallowCopyOfInputs)
(JNIEnv* env, jobject object, jlong native_pointer_to_stroke) {
  const Stroke* stroke = GetStroke(native_pointer_to_stroke);
  const StrokeInputBatch& stroke_batch = stroke->GetInputs();
  return reinterpret_cast<jlong>(new StrokeInputBatch(stroke_batch));
}

// Make a heap-allocated shallow (doesn't replicate all the individual meshes)
// copy of the `PartitionedMesh` belonging to this `Stroke`. Return the raw
// pointer to this copy, so that it can be wrapped by a JVM `PartitionedMesh`,
// which is responsible for freeing the copy when it is garbage collected and
// finalized.
JNI_METHOD(strokes, StrokeNative, jlong, newShallowCopyOfShape)
(JNIEnv* env, jobject object, jlong native_pointer_to_stroke) {
  const Stroke* stroke = GetStroke(native_pointer_to_stroke);
  const PartitionedMesh& stroke_shape = stroke->GetShape();
  PartitionedMesh* shallow_copy = new PartitionedMesh(stroke_shape);
  return reinterpret_cast<jlong>(shallow_copy);
}

// Free the given `Stroke`.
JNI_METHOD(strokes, StrokeNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer_to_stroke) {
  delete GetStroke(native_pointer_to_stroke);
}

}  // extern "C"
