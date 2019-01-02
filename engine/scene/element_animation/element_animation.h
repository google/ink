/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_SCENE_ELEMENT_ANIMATION_ELEMENT_ANIMATION_H_
#define INK_ENGINE_SCENE_ELEMENT_ANIMATION_ELEMENT_ANIMATION_H_

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/scene/element_animation/element_animation_controller.h"
#include "ink/engine/scene/graph/scene_graph.h"
#include "ink/engine/scene/types/element_id.h"
#include "ink/engine/util/animation/animation.h"
#include "ink/engine/util/animation/animation_curve.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/proto/animations_portable_proto.pb.h"

namespace ink {

void RunElementAnimation(const proto::ElementAnimation& proto_anim,
                         std::shared_ptr<SceneGraph> graph,
                         std::shared_ptr<ElementAnimationController> elem_ac);

std::unique_ptr<Animation> MakeColorElementAnimation(
    const ElementId& id, const proto::ColorAnimation& proto_anim,
    std::shared_ptr<SceneGraph> graph);

std::unique_ptr<Animation> MakeScaleElementAnimation(
    const ElementId& id, const proto::ScaleAnimation& proto_anim,
    std::shared_ptr<SceneGraph> graph);

}  // namespace ink

#endif  // INK_ENGINE_SCENE_ELEMENT_ANIMATION_ELEMENT_ANIMATION_H_
