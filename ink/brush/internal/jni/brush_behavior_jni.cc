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

#include <utility>
#include <variant>
#include <vector>

#include "absl/functional/overload.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/status_jni_helper.h"

namespace {

using ::ink::BrushBehavior;
using ::ink::brush_internal::ValidateBrushBehaviorTopLevel;
using ::ink::jni::JStringToStdString;
using ::ink::jni::ThrowExceptionFromStatus;
using ::ink::native::CastToBrushBehavior;
using ::ink::native::CastToBrushBehaviorNode;
using ::ink::native::DeleteNativeBrushBehavior;
using ::ink::native::NewNativeBrushBehavior;
using ::ink::native::NewNativeBrushBehaviorNode;

static constexpr int kSourceNode = 0;
static constexpr int kConstantNode = 1;
static constexpr int kNoiseNode = 2;
static constexpr int kToolTypeFilterNode = 3;
static constexpr int kDampingNode = 4;
static constexpr int kResponseNode = 5;
static constexpr int kIntegralNode = 6;
static constexpr int kBinaryOpNode = 7;
static constexpr int kInterpolationNode = 8;
static constexpr int kTargetNode = 9;
static constexpr int kPolarTargetNode = 10;

}  // namespace

extern "C" {

JNI_METHOD(brush, BrushBehaviorNative, jlong, createFromOrderedNodes)
(JNIEnv* env, jobject thiz, jlongArray node_native_pointer_array,
 jstring developer_comment) {
  std::vector<BrushBehavior::Node> nodes;
  const jsize num_nodes = env->GetArrayLength(node_native_pointer_array);
  nodes.reserve(num_nodes);
  jlong* node_pointers =
      env->GetLongArrayElements(node_native_pointer_array, nullptr);
  ABSL_CHECK(node_pointers != nullptr);
  for (jsize i = 0; i < num_nodes; ++i) {
    nodes.push_back(CastToBrushBehaviorNode(node_pointers[i]));
  }
  env->ReleaseLongArrayElements(
      node_native_pointer_array, node_pointers,
      // No need to copy back the array, which is not modified.
      JNI_ABORT);
  BrushBehavior behavior{
      .nodes = std::move(nodes),
      .developer_comment = JStringToStdString(env, developer_comment),
  };
  if (absl::Status status = ValidateBrushBehaviorTopLevel(behavior);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  return NewNativeBrushBehavior(std::move(behavior));
}

JNI_METHOD(brush, BrushBehaviorNative, void, free)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  DeleteNativeBrushBehavior(native_pointer);
}

JNI_METHOD(brush, BrushBehaviorNative, jint, getNodeCount)
(JNIEnv* env, jobject thiz, jlong native_pointer) {
  return CastToBrushBehavior(native_pointer).nodes.size();
}

JNI_METHOD(brush, BrushBehaviorNative, jint, getNodeTypeInt)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  constexpr auto visitor = absl::Overload{
      [](const BrushBehavior::SourceNode&) { return kSourceNode; },
      [](const BrushBehavior::ConstantNode&) { return kConstantNode; },
      [](const BrushBehavior::NoiseNode&) { return kNoiseNode; },
      [](const BrushBehavior::ToolTypeFilterNode&) {
        return kToolTypeFilterNode;
      },
      [](const BrushBehavior::DampingNode&) { return kDampingNode; },
      [](const BrushBehavior::ResponseNode&) { return kResponseNode; },
      [](const BrushBehavior::IntegralNode&) { return kIntegralNode; },
      [](const BrushBehavior::BinaryOpNode&) { return kBinaryOpNode; },
      [](const BrushBehavior::InterpolationNode&) {
        return kInterpolationNode;
      },
      [](const BrushBehavior::TargetNode&) { return kTargetNode; },
      [](const BrushBehavior::PolarTargetNode&) { return kPolarTargetNode; }};
  return std::visit(visitor, CastToBrushBehavior(native_pointer).nodes[index]);
}

JNI_METHOD(brush, BrushBehaviorNative, jstring, getDeveloperComment)
(JNIEnv* env, jobject object, jlong native_pointer) {
  const BrushBehavior& brush_behavior = CastToBrushBehavior(native_pointer);
  return env->NewStringUTF(brush_behavior.developer_comment.c_str());
}

JNI_METHOD(brush, BrushBehaviorNative, jlong, newCopyOfNode)
(JNIEnv* env, jobject thiz, jlong native_pointer, jint index) {
  return NewNativeBrushBehaviorNode(
      CastToBrushBehavior(native_pointer).nodes[index]);
}

}  // extern "C"
