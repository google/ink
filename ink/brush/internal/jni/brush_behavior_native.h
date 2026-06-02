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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_BEHAVIOR_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_BEHAVIOR_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t BrushBehaviorNative_createFromOrderedNodes(
    void* jni_env_pass_through, const int64_t* node_native_pointers,
    int num_nodes, const char* developer_comment,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

void BrushBehaviorNative_free(int64_t native_ptr);

int BrushBehaviorNative_getNodeCount(int64_t native_ptr);

int BrushBehaviorNative_getNodeTypeInt(int64_t native_ptr, int index);

const char* BrushBehaviorNative_getDeveloperComment(int64_t native_ptr);

int64_t BrushBehaviorNative_newCopyOfNode(int64_t native_ptr, int index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_BEHAVIOR_NATIVE_H_
