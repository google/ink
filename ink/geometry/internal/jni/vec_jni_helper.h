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

#ifndef INK_GEOMETRY_INTERNAL_JNI_VEC_JNI_HELPER_H_
#define INK_GEOMETRY_INTERNAL_JNI_VEC_JNI_HELPER_H_

#include <jni.h>

#include "ink/geometry/internal/jni/affine_transform_native.h"
#include "ink/geometry/internal/jni/box_native.h"
#include "ink/geometry/internal/jni/mesh_native.h"
#include "ink/geometry/internal/jni/parallelogram_native.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native.h"
#include "ink/geometry/internal/jni/vec_native.h"
#include "ink/geometry/point.h"
#include "ink/geometry/vec.h"

namespace ink::jni {

// Calls back into the JVM to create a new ImmutableVec object
// with the provided input. The caller must check if an exception was thrown by
// this call, e.g. with env->ExceptionCheck(). If an exception was thrown, the
// caller must bail out instead of continuing execution.
jobject CreateJImmutableVecOrThrow(JNIEnv* env, float x, float y);
jobject CreateJImmutableVecOrThrow(JNIEnv* env, const Vec& vec);
jobject CreateJImmutableVecOrThrow(JNIEnv* env, const Point& point);
jobject CreateJImmutableVecOrThrow(JNIEnv* env, const VecNative_Vec& vec);
jobject CreateJImmutableVecOrThrow(JNIEnv* env, const BoxNative_Vec& vec);
jobject CreateJImmutableVecOrThrow(JNIEnv* env,
                                   const ParallelogramNative_Vec& vec);
jobject CreateJImmutableVecOrThrow(JNIEnv* env,
                                   const AffineTransformNative_Vec& vec);

// Calls back into the JVM to populate an existing MutableVec object
// with the provided input. The caller must check if an exception was thrown by
// this call, e.g. with env->ExceptionCheck(). If an exception was thrown, the
// caller must bail out instead of continuing execution.
void FillJMutableVecOrThrow(JNIEnv* env, float x, float y, jobject mutable_vec);
void FillJMutableVecOrThrow(JNIEnv* env, const Vec& vec, jobject mutable_vec);
void FillJMutableVecOrThrow(JNIEnv* env, const Point& point,
                            jobject mutable_vec);
void FillJMutableVecOrThrow(JNIEnv* env, const VecNative_Vec& vec,
                            jobject mutable_vec);
void FillJMutableVecOrThrow(JNIEnv* env, const BoxNative_Vec& vec,
                            jobject mutable_vec);
void FillJMutableVecOrThrow(JNIEnv* env, const ParallelogramNative_Vec& vec,
                            jobject mutable_vec);
void FillJMutableVecOrThrow(JNIEnv* env, const MeshNative_Vec& vec,
                            jobject mutable_vec);
void FillJMutableVecOrThrow(JNIEnv* env, const PartitionedMeshNative_Vec& vec,
                            jobject mutable_vec);

}  // namespace ink::jni

#endif  // INK_GEOMETRY_INTERNAL_JNI_VEC_JNI_HELPER_H_
