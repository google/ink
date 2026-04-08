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

#ifndef INK_GEOMETRY_INTERNAL_JNI_MESH_NATIVE_H_
#define INK_GEOMETRY_INTERNAL_JNI_MESH_NATIVE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// C-compatible library header for Kotlin-native bindings.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x_min;
  float y_min;
  float x_max;
  float y_max;
} MeshNative_Box;

typedef struct {
  float x;
  float y;
} MeshNative_Vec;

// The native_ptr parameter of these methods contains the raw pointer to a
// C++ Mesh object stored in Kotlin on Mesh.nativePointer.

// Frees a Kotlin Mesh.nativePointer.
void MeshNative_free(int64_t native_ptr);

// Creates a new heap-allocated empty `Mesh` and returns a pointer to it as
// int64_t, suitable for wrapping in a Kotlin Mesh.
int64_t MeshNative_createEmpty();

// Takes a raw pointer to another `Mesh` as int64_t, creates a new
// heap-allocated copy of that, and returns a pointer to it as int64_t, suitable
// fors wrapping in a Kotlin Mesh.
int64_t MeshNative_newCopy(int64_t other_native_ptr);

int MeshNative_getVertexCount(int64_t native_ptr);

int MeshNative_getVertexStride(int64_t native_ptr);

int MeshNative_getTriangleCount(int64_t native_ptr);

int MeshNative_getAttributeCount(int64_t native_ptr);

bool MeshNative_isEmpty(int64_t native_ptr);

// Can only be called if MeshNative_isEmpty returns false.
MeshNative_Box MeshNative_getBounds(int64_t native_ptr);

int MeshNative_fillAttributeUnpackingParams(int64_t native_ptr,
                                            int attribute_index, float* offsets,
                                            float* scales);

int64_t MeshNative_newCopyOfFormat(int64_t native_ptr);

MeshNative_Vec MeshNative_getVertexPosition(int64_t native_ptr,
                                            int vertex_index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_GEOMETRY_INTERNAL_JNI_MESH_NATIVE_H_
