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

#include "ink/engine/scene/unsafe_scene_helper.h"

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/colors/colors.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/public/proto_validators.h"
#include "ink/engine/rendering/compositing/live_renderer.h"
#include "ink/engine/rendering/gl_managers/gl_resource_manager.h"
#include "ink/engine/scene/root_controller.h"
#include "ink/engine/scene/types/source_details.h"
#include "ink/engine/util/floats.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {

static constexpr float kMinBorderTextureScale = 0.00001;
static constexpr float kMaxBorderTextureScale = 10000;

Status UnsafeSceneHelper::AddElement(
    const proto::ElementBundle& unsafe_bundle, const UUID& below_uuid,
    const proto::SourceDetails& source_details_proto) {
  INK_RETURN_UNLESS(ValidateProtoForAdd(unsafe_bundle));
  SourceDetails source_details;
  EXPECT(util::ReadFromProto(source_details_proto, &source_details));
  return root_controller_->AddElementBelow(unsafe_bundle, source_details,
                                           below_uuid);
}

void UnsafeSceneHelper::ElementsAdded(
    const proto::ElementBundleAdds& unsafe_bundle_adds,
    const proto::SourceDetails& sourceDetails) {
  if (sourceDetails.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }

  for (const auto& add : unsafe_bundle_adds.element_bundle_add()) {
    AddElement(add.element_bundle(), add.below_uuid(), sourceDetails)
        .IgnoreError();
  }
}

void UnsafeSceneHelper::ElementsRemoved(
    const proto::ElementIdList& removedIds,
    const proto::SourceDetails& sourceDetails) {
  if (sourceDetails.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }

  SourceDetails sceneSourceDetails;
  EXPECT(util::ReadFromProto(sourceDetails, &sceneSourceDetails));
  for (int i = 0; i < removedIds.uuid_size(); ++i) {
    auto uuid = removedIds.uuid(i);
    SLOG(SLOG_DATA_FLOW, "Got Engine RemoveElement for UUID: $0", uuid);
    root_controller_->RemoveElement(uuid, sceneSourceDetails);
  }
}

void UnsafeSceneHelper::ElementsReplaced(
    const proto::ElementBundleReplace& replace,
    const proto::SourceDetails& source_details) {
  if (source_details.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }

  proto::ElementBundleReplace validated_replace;
  auto* validated_adds = validated_replace.mutable_elements_to_add();
  for (const auto& bundle_add :
       replace.elements_to_add().element_bundle_add()) {
    const auto& bundle = bundle_add.element_bundle();
    const auto& below_uuid = bundle_add.below_uuid();
    if (ValidateProto(bundle) &&
        (below_uuid == kInvalidUUID || is_valid_uuid(below_uuid))) {
      *validated_adds->add_element_bundle_add() = bundle_add;
    }
  }

  auto* validated_removes = validated_replace.mutable_elements_to_remove();
  for (const auto& remove : replace.elements_to_remove().uuid()) {
    if (is_valid_uuid(remove)) validated_removes->add_uuid(remove);
  }

  SourceDetails scene_source_details;
  EXPECT(util::ReadFromProto(source_details, &scene_source_details));
  root_controller_->ReplaceElements(validated_replace, scene_source_details);
}

template <typename SceneValueType, typename Mutations>
void UnsafeSceneHelper::MutateElements(
    const Mutations& unsafe_mutations,
    const proto::SourceDetails source_details,
    std::function<bool(const typename Mutations::Mutation& mutation,
                       typename std::vector<SceneValueType>::iterator)>
        read_value_func,
    std::function<void(RootController*, const std::vector<UUID>&,
                       const std::vector<SceneValueType>&,
                       const SourceDetails&)>
        root_controller_func) {
  if (source_details.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }

  if (!ValidateProto(unsafe_mutations)) {
    SLOG(SLOG_ERROR, "Unable to validate proto.");
    return;
  }

  int size = unsafe_mutations.mutation_size();

  std::vector<UUID> uuids(size);
  std::vector<SceneValueType> values(size);

  for (int i = 0; i < size; ++i) {
    const auto mutation = unsafe_mutations.mutation(i);
    uuids[i] = mutation.uuid();
    bool read_success = read_value_func(mutation, values.begin() + i);
    if (!read_success) {
      SLOG(SLOG_ERROR, "Failed to read value from mutation with index $0", i);
      return;
    }
  }
  SourceDetails scene_source_details;
  EXPECT(util::ReadFromProto(source_details, &scene_source_details));
  root_controller_func(root_controller_, uuids, values, scene_source_details);
}

void UnsafeSceneHelper::ElementsTransformMutated(
    const proto::ElementTransformMutations& unsafe_mutations,
    const proto::SourceDetails& source_details) {
  MutateElements<glm::mat4>(
      unsafe_mutations, source_details,
      [](const proto::ElementTransformMutations::Mutation& mutation,
         std::vector<glm::mat4>::iterator val_iter) {
        return util::ReadFromProto(mutation.transform(), &*val_iter).ok();
      },
      &RootController::SetTransforms);
}

void UnsafeSceneHelper::ElementsVisibilityMutated(
    const proto::ElementVisibilityMutations& unsafe_mutations,
    const proto::SourceDetails& source_details) {
  MutateElements<bool>(
      unsafe_mutations, source_details,
      [](const proto::ElementVisibilityMutations::Mutation& mutation,
         std::vector<bool>::iterator val_iter) {
        *val_iter = mutation.visibility();
        return true;
      },
      &RootController::SetVisibilities);
}

void UnsafeSceneHelper::ElementsOpacityMutated(
    const proto::ElementOpacityMutations& unsafe_mutations,
    const proto::SourceDetails& source_details) {
  MutateElements<int>(
      unsafe_mutations, source_details,
      [](const proto::ElementOpacityMutations::Mutation& mutation,
         std::vector<int>::iterator val_iter) {
        *val_iter = mutation.opacity();
        return true;
      },
      &RootController::SetOpacities);
}

void UnsafeSceneHelper::ElementsZOrderMutated(
    const proto::ElementZOrderMutations& unsafe_mutations,
    const proto::SourceDetails& source_details) {
  MutateElements<UUID>(
      unsafe_mutations, source_details,
      [](const proto::ElementZOrderMutations::Mutation& mutation,
         std::vector<UUID>::iterator val_iter) {
        *val_iter = mutation.below_uuid();
        return true;
      },
      &RootController::ChangeZOrders);
}

void UnsafeSceneHelper::PageBoundsChanged(
    const ink::proto::Rect& unsafe_bounds,
    const ink::proto::SourceDetails& source_details) {
  if (source_details.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }
  Rect page_bounds;
  if (!util::ReadFromProto(unsafe_bounds, &page_bounds)) {
    SLOG(SLOG_ERROR, "malformed bounds");
    return;
  }
  SourceDetails source_details_internal;
  EXPECT(util::ReadFromProto(source_details, &source_details_internal));
  SetPageBounds(page_bounds, source_details_internal);
}

void UnsafeSceneHelper::BackgroundColorChanged(
    const ink::proto::Color& background_color,
    const ink::proto::SourceDetails& source_details) {
  if (source_details.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }
  SLOG(SLOG_DATA_FLOW, "Setting background color");
  glm::vec4 color = UintToVec4ARGB(background_color.argb());
  SetBackgroundColor(color);
}

void UnsafeSceneHelper::BackgroundImageChanged(
    const ink::proto::BackgroundImageInfo& image,
    const ink::proto::SourceDetails& source_details) {
  if (source_details.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }
  SLOG(SLOG_BOUNDS, "BackgroundImageChanged");
  SLOG(SLOG_DATA_FLOW, "Setting background image to uri $0.", image.uri());
  Rect bounds;
  if (image.has_bounds()) {
    if (!util::ReadFromProto(image.bounds(), &bounds)) {
      SLOG(SLOG_ERROR, "Malformed firstInstanceBounds");
      return;
    }
    SLOG(SLOG_BOUNDS, "bg image has explicit bounds of $0", bounds);
  } else if (root_controller_->service<PageBounds>()->HasBounds()) {
    // Set the first_instance_bounds to the page_bounds if it is unset.
    bounds = root_controller_->service<PageBounds>()->Bounds();
    SLOG(SLOG_BOUNDS, "bg image does not have bounds; setting to $0", bounds);
  }
  SetBackgroundImage(bounds, image.uri());
}

void UnsafeSceneHelper::BorderChanged(
    const ink::proto::Border& border,
    const ink::proto::SourceDetails& source_details) {
  if (source_details.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }
  if (!border.has_uri()) {
    SLOG(SLOG_DATA_FLOW, "clearing page border");
    ClearBorder();
    return;
  }
  SLOG(SLOG_DATA_FLOW, "Setting page border to uri: $0", border.uri());
  float scale = border.scale();
  std::string uri;
  if (!border.has_uri() || border.uri() == "") {
    SLOG(SLOG_ERROR, "could not set page border, missing param uri");
    return;
  } else {
    uri = border.uri();
  }
  SetBorder(uri, scale);
}

void UnsafeSceneHelper::GridChanged(
    const proto::GridInfo& grid_info,
    const ink::proto::SourceDetails& source_details) {
  using util::floats::kFloatSafeMax;
  using util::floats::kFloatZeroTol;

  if (source_details.origin() == proto::SourceDetails::ENGINE) {
    // Ignore anything with origin ENGINE since it was already processed.
    return;
  }
  if (!grid_info.has_uri() || grid_info.uri().empty()) {
    // No URI specified, clear the grid.
    root_controller_->ClearGrid();
    return;
  }

  if (!BoundsCheckIncInc(grid_info.size_world(), kFloatZeroTol,
                         kFloatSafeMax)) {
    SLOG(SLOG_ERROR, "Could not set grid, invalid size ($0)",
         grid_info.size_world());
    return;
  }
  if (grid_info.has_origin() &&
      !(BoundsCheckIncInc(grid_info.origin().x(), -kFloatSafeMax,
                          kFloatSafeMax) &&
        BoundsCheckIncInc(grid_info.origin().y(), -kFloatSafeMax,
                          kFloatSafeMax))) {
    SLOG(SLOG_ERROR,
         "Could not set grid, the absolute value of the origin is too "
         "large "
         "($0, $1)",
         grid_info.origin().x(), grid_info.origin().y());
    return;
  }

  root_controller_->SetGrid(grid_info);
}  // namespace ink

void UnsafeSceneHelper::SetBackgroundColor(const glm::vec4& color) {
  SLOG(SLOG_DATA_FLOW, "Setting background to solid color (r,g,b,a)=$0", color);
  auto color_premult = RGBtoRGBPremultiplied(color);
  auto glr = root_controller_->service<GLResourceManager>();
  glr->background_state->SetToColor(glr->texture_manager.get(), color_premult);
  root_controller_->service<LiveRenderer>()->Invalidate();
}

void UnsafeSceneHelper::SetBackgroundImage(const Rect& bounds,
                                           const std::string& uri) {
  TextureInfo bg_texture(uri);
  auto glr = root_controller_->service<GLResourceManager>();
  glr->background_state->SetToImage(glr->texture_manager.get(), bg_texture,
                                    bounds);
  root_controller_->service<LiveRenderer>()->Invalidate();
}

void UnsafeSceneHelper::SetPageBounds(const Rect& bounds,
                                      const SourceDetails& source_details) {
  if (root_controller_->service<PageBounds>()->SetBounds(bounds,
                                                         source_details)) {
    if (bounds.Area() > 0) {
      // If there's a background image with no explicit world coordinates,
      // set it.
      auto* bs = root_controller_->service<GLResourceManager>()
                     ->background_state.get();
      ImageBackgroundState* ibs;
      if (bs->GetImage(&ibs) && !ibs->HasFirstInstanceWorldCoords()) {
        ibs->SetFirstInstanceWorldCoords(bounds);
      }
      // Move the camera to display the world bounds.
      root_controller_->service<CameraController>()->LookAt(bounds);
    }
    root_controller_->service<LiveRenderer>()->Invalidate();
  }
}

void UnsafeSceneHelper::SetBorder(const std::string& uri, float scale) {
  if (!BoundsCheckIncInc(scale, kMinBorderTextureScale,
                         kMaxBorderTextureScale)) {
    SLOG(SLOG_ERROR, "could not set page border, invalid scale ($0)", scale);
    return;
  }
  root_controller_->SetPageBorder(uri, scale);
}

void UnsafeSceneHelper::ClearBorder() { root_controller_->ClearPageBorder(); }

}  // namespace ink
