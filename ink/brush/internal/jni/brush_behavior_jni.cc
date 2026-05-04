// Copyright 2024 Google LLC
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

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/internal/jni/brush_behavior_native.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/status_jni_helper.h"

namespace {

using ::ink::BrushBehavior;
using ::ink::jni::JStringToStdString;
using ::ink::jni::ThrowExceptionFromStatusCallback;

}  // namespace

extern "C" {

JNI_METHOD(brush, BrushBehaviorNative, jlong, createFromOrderedNodes)
(JNIEnv* env, jobject thiz, jlongArray node_native_pointer_array,
 jstring developer_comment) {
  const jsize num_nodes = env->GetArrayLength(node_native_pointer_array);
  jlong* node_pointers =
      env->GetLongArrayElements(node_native_pointer_array, nullptr);
  ABSL_CHECK(node_pointers != nullptr);
  // Both `jlong` and `int64_t` are required to be 64-bit integers which JNI and
  // Kotlin-cinterop respectively both map to Kotlin `Long`. However, on MacOS
  // they represent two distinct (though equivalent) types, `jlong` is `long`
  // but `int64_t` is `long long`.
  static_assert(sizeof(jlong) == sizeof(int64_t));
  int64_t result = BrushBehaviorNative_createFromOrderedNodes(
      env, reinterpret_cast<int64_t*>(node_pointers), num_nodes,
      JStringToStdString(env, developer_comment).c_str(),
      &ThrowExceptionFromStatusCallback);
  env->ReleaseLongArrayElements(
      node_native_pointer_array, node_pointers,
      // No need to copy back the array, which is not modified.
      JNI_ABORT);
  return result;
}

JNI_METHOD(brush, BrushBehaviorNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  BrushBehaviorNative_free(native_pointer);
}

JNI_METHOD(brush, BrushBehaviorNative, jint, getNodeCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return BrushBehaviorNative_getNodeCount(native_pointer);
}

JNI_METHOD(brush, BrushBehaviorNative, jint, getNodeTypeInt)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushBehaviorNative_getNodeTypeInt(native_pointer, index);
}

JNI_METHOD(brush, BrushBehaviorNative, jstring, getDeveloperComment)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return env->NewStringUTF(
      BrushBehaviorNative_getDeveloperComment(native_pointer));
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, newCopyOfNode)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return BrushBehaviorNative_newCopyOfNode(native_pointer, index);
}

}  // extern "C"
