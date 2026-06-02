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

#include "ink/brush/internal/jni/brush_behavior_native.h"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/functional/overload.h"
#include "absl/status/status.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/internal/jni/brush_native_helper.h"

namespace {
using ::ink::BrushBehavior;
using ::ink::brush_internal::ValidateBrushBehaviorTopLevel;
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

int64_t BrushBehaviorNative_createFromOrderedNodes(
    void* jni_env_pass_through, const int64_t* node_native_pointers,
    int num_nodes, const char* developer_comment,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str)) {
  BrushBehavior brush_behavior{.developer_comment = developer_comment};
  brush_behavior.nodes.reserve(num_nodes);
  for (int i = 0; i < num_nodes; ++i) {
    brush_behavior.nodes.push_back(
        CastToBrushBehaviorNode(node_native_pointers[i]));
  }
  if (absl::Status status = ValidateBrushBehaviorTopLevel(brush_behavior);
      !status.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(status.code()),
                               status.ToString().c_str());
    return 0;
  }
  return NewNativeBrushBehavior(std::move(brush_behavior));
}

void BrushBehaviorNative_free(int64_t native_ptr) {
  DeleteNativeBrushBehavior(native_ptr);
}

int BrushBehaviorNative_getNodeCount(int64_t native_ptr) {
  return CastToBrushBehavior(native_ptr).nodes.size();
}

int BrushBehaviorNative_getNodeTypeInt(int64_t native_ptr, int index) {
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
  return std::visit(visitor, CastToBrushBehavior(native_ptr).nodes[index]);
}

const char* BrushBehaviorNative_getDeveloperComment(int64_t native_ptr) {
  return CastToBrushBehavior(native_ptr).developer_comment.c_str();
}

int64_t BrushBehaviorNative_newCopyOfNode(int64_t native_ptr, int index) {
  return NewNativeBrushBehaviorNode(
      CastToBrushBehavior(native_ptr).nodes[index]);
}

}  // extern "C"
