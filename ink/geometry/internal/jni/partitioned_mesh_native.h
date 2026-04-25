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

#ifndef INK_GEOMETRY_INTERNAL_JNI_PARTITIONED_MESH_NATIVE_H_
#define INK_GEOMETRY_INTERNAL_JNI_PARTITIONED_MESH_NATIVE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// C-compatible library header for Kotlin-native bindings.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x;
  float y;
} PartitionedMeshNative_Vec;

typedef struct {
  float x_min;
  float y_min;
  float x_max;
  float y_max;
} PartitionedMeshNative_Box;

// The native_ptr parameter of these methods contains the raw pointer to a
// C++ PartitionedMesh object stored in Kotlin on PartitionedMesh.nativePointer.

// Frees a Kotlin PartitionedMesh.nativePointer.
void PartitionedMeshNative_free(int64_t native_ptr);

// Creates a new heap-allocated empty `PartitionedMesh` and returns a pointer
// to it as int64_t, suitable for wrapping in a Kotlin PartitionedMesh.
int64_t PartitionedMeshNative_createEmptyForTesting();

// Creates a new heap-allocated `PartitionedMesh` from a triangle and returns a
// pointer to it as int64_t, suitable for wrapping in a Kotlin PartitionedMesh.
int64_t PartitionedMeshNative_createFromTriangleForTesting(
    float triangle_p0_x, float triangle_p0_y, float triangle_p1_x,
    float triangle_p1_y, float triangle_p2_x, float triangle_p2_y);

void PartitionedMeshNative_fillRenderGroupMeshPointers(int64_t native_ptr,
                                                       int group_index,
                                                       int64_t* out_pointers);

int PartitionedMeshNative_getRenderGroupCount(int64_t native_ptr);

// Returns a raw pointer to a new heap-allocated 'MeshFormat' suitable for
// wrapping in a Kotlin MeshFormat.
int64_t PartitionedMeshNative_newCopyOfRenderGroupFormat(int64_t native_ptr,
                                                         int group_index);

int PartitionedMeshNative_getOutlineCount(int64_t native_ptr, int group_index);

int PartitionedMeshNative_getOutlineVertexCount(int64_t native_ptr,
                                                int group_index,
                                                int outline_index);

PartitionedMeshNative_Vec PartitionedMeshNative_getOutlineVertexPosition(
    int64_t native_ptr, int group_index, int outline_index,
    int outline_vertex_index);

bool PartitionedMeshNative_isEmpty(int64_t native_ptr);

// Can only be called if PartitionedMeshNative_isEmpty returns false.
PartitionedMeshNative_Box PartitionedMeshNative_getBounds(int64_t native_ptr);

float PartitionedMeshNative_triangleCoverage(
    int64_t native_ptr, float triangle_p0_x, float triangle_p0_y,
    float triangle_p1_x, float triangle_p1_y, float triangle_p2_x,
    float triangle_p2_y, float a, float b, float c, float d, float e, float f);

float PartitionedMeshNative_boxCoverage(int64_t native_ptr, float x_min,
                                        float y_min, float x_max, float y_max,
                                        float a, float b, float c, float d,
                                        float e, float f);

float PartitionedMeshNative_parallelogramCoverage(
    int64_t native_ptr, float center_x, float center_y, float width,
    float height, float angle_radian, float shear_factor, float a, float b,
    float c, float d, float e, float f);

// other_ptr contains the pointer address of another PartitionedMesh.
float PartitionedMeshNative_partitionedMeshCoverage(int64_t native_ptr,
                                                    int64_t other_ptr, float a,
                                                    float b, float c, float d,
                                                    float e, float f);

bool PartitionedMeshNative_triangleCoverageIsGreaterThan(
    int64_t native_ptr, float triangle_p0_x, float triangle_p0_y,
    float triangle_p1_x, float triangle_p1_y, float triangle_p2_x,
    float triangle_p2_y, float threshold, float a, float b, float c, float d,
    float e, float f);

bool PartitionedMeshNative_boxCoverageIsGreaterThan(
    int64_t native_ptr, float x_min, float y_min, float x_max, float y_max,
    float threshold, float a, float b, float c, float d, float e, float f);

bool PartitionedMeshNative_parallelogramCoverageIsGreaterThan(
    int64_t native_ptr, float center_x, float center_y, float width,
    float height, float angle_radian, float shear_factor, float threshold,
    float a, float b, float c, float d, float e, float f);

bool PartitionedMeshNative_partitionedMeshCoverageIsGreaterThan(
    int64_t native_ptr, int64_t other_ptr, float threshold, float a, float b,
    float c, float d, float e, float f);

void PartitionedMeshNative_initializeSpatialIndex(int64_t native_ptr);

bool PartitionedMeshNative_isSpatialIndexInitialized(int64_t native_ptr);

int PartitionedMeshNative_getRenderGroupMeshCount(int64_t native_ptr,
                                                  int group_index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_GEOMETRY_INTERNAL_JNI_PARTITIONED_MESH_NATIVE_H_
