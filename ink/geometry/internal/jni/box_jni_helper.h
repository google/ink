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

#ifndef INK_GEOMETRY_INTERNAL_JNI_BOX_JNI_HELPER_H_
#define INK_GEOMETRY_INTERNAL_JNI_BOX_JNI_HELPER_H_

#include <jni.h>

#include "ink/geometry/internal/jni/mesh_native.h"
#include "ink/geometry/internal/jni/parallelogram_native.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native.h"
#include "ink/geometry/rect.h"

namespace ink::jni {

// Calls back into the JVM to create a new ImmutableBox object from the provided
// input. The caller must check if an exception was thrown by this call, e.g.
// with env->ExceptionCheck(). If an exception was thrown, the caller must bail
// out early instead of continuing execution.
jobject CreateJImmutableBoxOrThrow(JNIEnv* env, float x_min, float y_min,
                                   float x_max, float y_max);
jobject CreateJImmutableBoxOrThrow(JNIEnv* env, const Rect& rect);
jobject CreateJImmutableBoxOrThrow(JNIEnv* env,
                                   const ParallelogramNative_Box& box);
jobject CreateJImmutableBoxOrThrow(JNIEnv* env, const MeshNative_Box& box);
jobject CreateJImmutableBoxOrThrow(JNIEnv* env,
                                   const PartitionedMeshNative_Box& box);

// Calls back into the JVM to populate an existing MutableBox object with the
// provided input. The caller must check if an exception was thrown by this
// call, e.g. with env->ExceptionCheck(). If an exception was thrown, the caller
// must bail out instead of continuing execution.
void FillJMutableBoxOrThrow(JNIEnv* env, float x_min, float y_min, float x_max,
                            float y_max, jobject mutable_box);
void FillJMutableBoxOrThrow(JNIEnv* env, const Rect& rect, jobject mutable_box);
void FillJMutableBoxOrThrow(JNIEnv* env, const ParallelogramNative_Box& box,
                            jobject mutable_box);

}  // namespace ink::jni

#endif  // INK_GEOMETRY_INTERNAL_JNI_BOX_JNI_HELPER_H_
