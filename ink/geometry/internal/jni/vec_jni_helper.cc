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

#include "ink/geometry/internal/jni/vec_jni_helper.h"

#include <jni.h>

#include "ink/geometry/internal/jni/affine_transform_native.h"
#include "ink/geometry/internal/jni/box_native.h"
#include "ink/geometry/internal/jni/mesh_native.h"
#include "ink/geometry/internal/jni/parallelogram_native.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native.h"
#include "ink/geometry/internal/jni/vec_native.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"
#include "ink/jni/internal/jni_jvm_interface.h"

namespace ink::jni {

jobject CreateJImmutableVecOrThrow(JNIEnv* env, float x, float y) {
  return env->NewObject(ClassImmutableVec(env), MethodImmutableVecInitXY(env),
                        x, y);
}

jobject CreateJImmutableVecOrThrow(JNIEnv* env, const Vec& vec) {
  return CreateJImmutableVecOrThrow(env, vec.x, vec.y);
}

jobject CreateJImmutableVecOrThrow(JNIEnv* env, const Point& point) {
  return CreateJImmutableVecOrThrow(env, point.x, point.y);
}

jobject CreateJImmutableVecOrThrow(JNIEnv* env, const VecNative_Vec& vec) {
  return CreateJImmutableVecOrThrow(env, vec.x, vec.y);
}

jobject CreateJImmutableVecOrThrow(JNIEnv* env, const BoxNative_Vec& vec) {
  return CreateJImmutableVecOrThrow(env, vec.x, vec.y);
}

jobject CreateJImmutableVecOrThrow(JNIEnv* env,
                                   const ParallelogramNative_Vec& vec) {
  return CreateJImmutableVecOrThrow(env, vec.x, vec.y);
}

jobject CreateJImmutableVecOrThrow(JNIEnv* env,
                                   const AffineTransformNative_Vec& vec) {
  return CreateJImmutableVecOrThrow(env, vec.x, vec.y);
}

void FillJMutableVecOrThrow(JNIEnv* env, float x, float y,
                            jobject mutable_vec) {
  env->CallVoidMethod(mutable_vec, MethodMutableVecSetX(env), x);
  if (env->ExceptionCheck()) return;
  env->CallVoidMethod(mutable_vec, MethodMutableVecSetY(env), y);
}

void FillJMutableVecOrThrow(JNIEnv* env, const Vec& vec, jobject mutable_vec) {
  FillJMutableVecOrThrow(env, vec.x, vec.y, mutable_vec);
}

void FillJMutableVecOrThrow(JNIEnv* env, const Point& point,
                            jobject mutable_vec) {
  FillJMutableVecOrThrow(env, point.x, point.y, mutable_vec);
}

void FillJMutableVecOrThrow(JNIEnv* env, const VecNative_Vec& vec,
                            jobject mutable_vec) {
  FillJMutableVecOrThrow(env, vec.x, vec.y, mutable_vec);
}

void FillJMutableVecOrThrow(JNIEnv* env, const BoxNative_Vec& vec,
                            jobject mutable_vec) {
  FillJMutableVecOrThrow(env, vec.x, vec.y, mutable_vec);
}

void FillJMutableVecOrThrow(JNIEnv* env, const ParallelogramNative_Vec& vec,
                            jobject mutable_vec) {
  FillJMutableVecOrThrow(env, vec.x, vec.y, mutable_vec);
}

void FillJMutableVecOrThrow(JNIEnv* env, const MeshNative_Vec& vec,
                            jobject mutable_vec) {
  FillJMutableVecOrThrow(env, vec.x, vec.y, mutable_vec);
}

void FillJMutableVecOrThrow(JNIEnv* env, const PartitionedMeshNative_Vec& vec,
                            jobject mutable_vec) {
  FillJMutableVecOrThrow(env, vec.x, vec.y, mutable_vec);
}

}  // namespace ink::jni
