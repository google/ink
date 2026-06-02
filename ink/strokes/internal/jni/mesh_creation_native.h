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

#ifndef INK_STROKES_INTERNAL_JNI_MESH_CREATION_NATIVE_H_
#define INK_STROKES_INTERNAL_JNI_MESH_CREATION_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Creates a closed shape from the given stroke input batch and returns the
// native pointer to the resulting PartitionedMesh.
int64_t MeshCreationNative_createClosedShapeFromStrokeInputBatch(
    void* jni_env_pass_through, int64_t stroke_input_batch_native_pointer,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_STROKES_INTERNAL_JNI_MESH_CREATION_NATIVE_H_
