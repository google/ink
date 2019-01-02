// Copyright 2018 Google LLC
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

#include "ink/engine/scene/element_animation/element_animation.h"

#include <cmath>

#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/scene/element_animation/element_animation_controller.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/util/animation/animation_curve.h"
#include "ink/engine/util/animation/fixed_interp_animation.h"
#include "ink/engine/util/animation/parallel_animation.h"

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

namespace ink {

void RunElementAnimation(const proto::ElementAnimation& proto_anim,
                         std::shared_ptr<SceneGraph> graph,
                         std::shared_ptr<ElementAnimationController> elem_ac) {
  std::unique_ptr<ParallelAnimation> anim(new ParallelAnimation());

  ElementId id = graph->ElementIdFromUUID(proto_anim.uuid());

  if (proto_anim.has_color_animation()) {
    const proto::ColorAnimation& color_anim = proto_anim.color_animation();
    if (std::isnan(color_anim.duration()) || color_anim.duration() <= 0) {
      SLOG(SLOG_ERROR, "corrupted or negative duration");
    } else {
      anim->Add(MakeColorElementAnimation(id, color_anim, graph));
    }
  }

  if (proto_anim.has_scale_animation()) {
    std::unique_ptr<Animation> scaler =
        MakeScaleElementAnimation(id, proto_anim.scale_animation(), graph);
    if (scaler) {
      anim->Add(std::move(scaler));
    }
  }

  elem_ac->PushAnimation(id, std::move(anim));

  if (proto_anim.has_next()) {
    RunElementAnimation(proto_anim.next(), graph, elem_ac);
  }
}

std::unique_ptr<Animation> MakeColorElementAnimation(
    const ElementId& id, const proto::ColorAnimation& color_anim,
    std::shared_ptr<SceneGraph> graph) {
  std::weak_ptr<SceneGraph> weak_graph(graph);
  vec4 target_color(RGBtoRGBPremultiplied(UintToVec4RGBA(color_anim.rgba())));
  auto curve = ReadFromProto(color_anim.curve());
  DurationS d(color_anim.duration());

  auto get = [id, weak_graph]() {
    if (auto graph = weak_graph.lock()) return graph->GetColor(id);
    return glm::vec4{0, 0, 0, 0};
  };

  auto set = [id, weak_graph](const vec4& c) {
    if (auto graph = weak_graph.lock())
      graph->SetColor(id, c, SourceDetails::EngineInternal());
  };

  auto finish = [id, target_color, weak_graph]() {
    if (auto graph = weak_graph.lock())
      graph->SetColor(id, target_color, SourceDetails::FromEngine());
  };

  std::unique_ptr<Animation> anim(new FixedInterpAnimation<vec4>(
      d, target_color, get, set, std::move(curve),
      DefaultInterpolator<vec4>()));
  anim->SetOnFinishedFn(finish);

  return anim;
}

std::unique_ptr<Animation> MakeScaleElementAnimation(
    const ElementId& id, const proto::ScaleAnimation& scale_anim,
    std::shared_ptr<SceneGraph> graph) {
  std::weak_ptr<SceneGraph> weak_graph(graph);
  auto curve = ReadFromProto(scale_anim.curve());
  DurationS d(scale_anim.duration());

  const ElementMetadata& metadata = graph->GetElementMetadata(id);
  if (metadata.id == kInvalidElementId) {
    SLOG(SLOG_ERROR, "cannot animate unknown id");
    return nullptr;
  }
  auto transform = metadata.group_transform;

  // Scale to be from the center of the stroke, not the origin.
  vec2 offset =
      geometry::Transform(graph->Mbr({id}).Center(), glm::inverse(transform));
  mat4 move = glm::translate(mat4{1}, -vec3(offset, 0));
  mat4 scaled =
      glm::scale(mat4{1}, vec3(scale_anim.scale_x(), scale_anim.scale_y(), 1));
  mat4 move_back = glm::translate(mat4{1}, vec3(offset, 0));
  mat4 target = transform * move_back * scaled * move;

  auto get = [id, weak_graph]() {
    if (auto graph = weak_graph.lock())
      return graph->GetElementMetadata(id).group_transform;
    return glm::mat4{1};
  };

  auto set = [id, weak_graph](const mat4& t) {
    if (auto graph = weak_graph.lock())
      graph->TransformElement(id, t, SourceDetails::EngineInternal());
  };

  auto finish = [id, target, weak_graph]() {
    if (auto graph = weak_graph.lock())
      graph->TransformElement(id, target, SourceDetails::FromEngine());
  };

  std::unique_ptr<Animation> anim(new FixedInterpAnimation<mat4>(
      d, target, get, set, std::move(curve), DefaultInterpolator<mat4>()));
  anim->SetOnFinishedFn(finish);

  return anim;
}

}  // namespace ink
