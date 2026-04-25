
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

#include "ink/geometry/internal/jni/box_jni_helper.h"

#include <jni.h>

#include "ink/geometry/internal/jni/mesh_native.h"
#include "ink/geometry/internal/jni/parallelogram_native.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native.h"
#include "ink/geometry/internal/jni/vec_jni_helper.h"
#include "ink/geometry/rect.h"
#include "ink/jni/internal/jni_jvm_interface.h"

namespace ink::jni {

jobject CreateJImmutableBoxOrThrow(JNIEnv* env, float x_min, float y_min,
                                   float x_max, float y_max) {
  return env->CallStaticObjectMethod(
      ClassImmutableBox(env), MethodImmutableBoxFromTwoPoints(env),
      CreateJImmutableVecOrThrow(env, x_min, y_min),
      CreateJImmutableVecOrThrow(env, x_max, y_max));
}

jobject CreateJImmutableBoxOrThrow(JNIEnv* env, const Rect& rect) {
  return CreateJImmutableBoxOrThrow(env, rect.XMin(), rect.YMin(), rect.XMax(),
                                    rect.YMax());
}

jobject CreateJImmutableBoxOrThrow(JNIEnv* env,
                                   const ParallelogramNative_Box& box) {
  return CreateJImmutableBoxOrThrow(env, box.xmin, box.ymin, box.xmax,
                                    box.ymax);
}

jobject CreateJImmutableBoxOrThrow(JNIEnv* env, const MeshNative_Box& box) {
  return CreateJImmutableBoxOrThrow(env, box.x_min, box.y_min, box.x_max,
                                    box.y_max);
}

jobject CreateJImmutableBoxOrThrow(JNIEnv* env,
                                   const PartitionedMeshNative_Box& box) {
  return CreateJImmutableBoxOrThrow(env, box.x_min, box.y_min, box.x_max,
                                    box.y_max);
}

void FillJMutableBoxOrThrow(JNIEnv* env, float x_min, float y_min, float x_max,
                            float y_max, jobject mutable_box) {
  env->CallObjectMethod(mutable_box, MethodMutableBoxSetXBounds(env), x_min,
                        x_max);
  if (env->ExceptionCheck()) return;
  env->CallObjectMethod(mutable_box, MethodMutableBoxSetYBounds(env), y_min,
                        y_max);
}

void FillJMutableBoxOrThrow(JNIEnv* env, const Rect& rect,
                            jobject mutable_box) {
  FillJMutableBoxOrThrow(env, rect.XMin(), rect.YMin(), rect.XMax(),
                         rect.YMax(), mutable_box);
}

void FillJMutableBoxOrThrow(JNIEnv* env, const ParallelogramNative_Box& box,
                            jobject mutable_box) {
  FillJMutableBoxOrThrow(env, box.xmin, box.ymin, box.xmax, box.ymax,
                         mutable_box);
}

}  // namespace ink::jni
