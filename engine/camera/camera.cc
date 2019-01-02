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

#include "ink/engine/camera/camera.h"

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "third_party/glm/glm/gtc/matrix_transform.hpp"
#include "ink/engine/geometry/algorithms/fuzzy_compare.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/primitives/matrix_utils.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/funcs/step_utils.h"
#include "ink/engine/util/funcs/utils.h"
#include "ink/engine/util/security.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

using glm::vec2;
using glm::vec3;
using glm::vec4;

namespace {
constexpr float kCmPerInch = 2.54f;
constexpr int kDeviceIndependentPixelsPerInch = 160;
constexpr int kMaxViewportSize = 100000;
constexpr int kMaxPPI = 10000;

void ReadViewportWithoutValidating(const proto::Viewport& proto, Camera* cam) {
  cam->SetPPI(proto.ppi());
  cam->SetScreenDim({proto.width(), proto.height()});
}

void ReadCameraPositionWithoutValidating(const proto::CameraPosition& proto,
                                         Camera* cam) {
  cam->SetPosition({proto.world_center().x(), proto.world_center().y()},
                   {proto.world_width(), proto.world_height()}, 0);
}

}  // namespace

Camera::Camera() { SetScreenDim({1000, 1000}); }

void Camera::SetScreenDim(glm::ivec2 size) {
  EXPECT(size.x > 0 && size.y > 0);
  auto old_center = WorldCenter();
  screen_dim_ = size;
  screen_to_device_ =
      Rect({0, 0}, screen_dim_).CalcTransformTo(Rect({-1, -1}, {1, 1}));
  Translate(old_center - WorldCenter());
}

float Camera::ScaleFactor() const {
  glm::vec2 scale = matrix_utils::GetScaleComponent(ScreenToWorld());
  ASSERT(scale.x > 0 && scale.x == scale.y);
  return scale.x;
}

bool Camera::WithinPrecisionBounds() const {
  return IsTransformSafeForPrecision(ScreenDim(), ScreenToWorld());
}

void Camera::SetPosition(glm::vec2 world_center, glm::vec2 world_dim,
                         float rotation_radians) {
  EXPECT(world_dim.x > 0 || world_dim.y > 0);
  float scale =
      std::max(world_dim.x / screen_dim_.x, world_dim.y / screen_dim_.y);
  screen_to_world_ =
      glm::translate(glm::mat4{1}, glm::vec3(world_center, 0)) *
      glm::rotate(glm::mat4{1}, rotation_radians, glm::vec3{0, 0, 1}) *
      glm::scale(glm::mat4{1}, glm::vec3{scale, scale, 1}) *
      glm::translate(glm::mat4{1}, -.5f * glm::vec3(screen_dim_, 0));
  RecalcMatrices();
}

void Camera::Translate(glm::vec2 world_translation) {
  screen_to_world_ =
      glm::translate(glm::mat4{1}, glm::vec3(world_translation, 0)) *
      screen_to_world_;
  RecalcMatrices();
}

void Camera::Scale(float factor, glm::vec2 world_scale_center) {
  EXPECT(factor > 0);
  glm::mat4 candidate_screen_to_world =
      matrix_utils::ScaleAboutPoint(factor, world_scale_center) *
      screen_to_world_;

  if (IsTransformSafeForPrecision(ScreenDim(), candidate_screen_to_world)) {
    screen_to_world_ = candidate_screen_to_world;
    RecalcMatrices();
  }
}

void Camera::Rotate(float angle_radians, glm::vec2 world_rotation_center) {
  screen_to_world_ =
      matrix_utils::RotateAboutPoint(angle_radians, world_rotation_center) *
      screen_to_world_;
  RecalcMatrices();
}

float Camera::Coverage(const float width) const {
  return util::Clamp(0.0000001f, 1.0f, width / WorldWindow().Width());
}

void Camera::RecalcMatrices() {
  ASSERT(matrix_utils::IsAffineTransform(screen_to_world_));
  ASSERT(matrix_utils::IsInvertible(screen_to_world_));
  // The screen-to-world transform must scale uniformly in both directions.
  ASSERT(screen_to_world_[0][0] * screen_to_world_[0][1] ==
         -screen_to_world_[1][0] * screen_to_world_[1][1]);
  // The screen-to-world transform must not be skewed.
  ASSERT(screen_to_world_[0][0] * screen_to_world_[1][0] ==
         -screen_to_world_[0][1] * screen_to_world_[1][1]);

  world_to_screen_ = glm::inverse(screen_to_world_);
  world_to_device_ = screen_to_device_ * world_to_screen_;

  // Check for NaN.
  ASSERT(world_to_screen_ == world_to_screen_);
  ASSERT(world_to_device_ == world_to_device_);

  SLOG(SLOG_CAMERA, "recalculating Camera matrices");
  SLOG(SLOG_CAMERA, "world_to_screen_:\n$0", world_to_screen_);
  SLOG(SLOG_CAMERA, "screen_to_device_:\n$0", screen_to_device_);
}

bool Camera::IsTransformSafeForPrecision(glm::ivec2 screen_dim,
                                         const glm::mat4& screen_to_world) {
  float eps = std::numeric_limits<float>::epsilon();
  float scale = matrix_utils::GetScaleComponent(screen_to_world).x;
  if (eps * scale * std::max(screen_dim.x, screen_dim.y) > 1) return false;

  Rect window_bounds =
      geometry::Transform(Rect({0, 0}, screen_dim), screen_to_world);
  return eps * std::abs(window_bounds.from.x) < scale &&
         eps * std::abs(window_bounds.from.y) < scale &&
         eps * std::abs(window_bounds.to.x) < scale &&
         eps * std::abs(window_bounds.to.y) < scale;
}

glm::vec2 Camera::ConvertPosition(glm::vec2 position, CoordType type_in,
                                  CoordType type_out) const {
  if (type_in == type_out) return position;

  switch (type_in) {
    case CoordType::kWorld:
      ASSERT(type_out == CoordType::kScreen);
      return geometry::Transform(position, WorldToScreen());
    case CoordType::kScreen:
      ASSERT(type_out == CoordType::kWorld);
      return geometry::Transform(position, ScreenToWorld());
  }
}

glm::vec2 Camera::ConvertVector(glm::vec2 vector, CoordType type_in,
                                CoordType type_out) const {
  if (type_in == type_out) return vector;

  switch (type_in) {
    case CoordType::kWorld:
      ASSERT(type_out == CoordType::kScreen);
      return glm::mat2(WorldToScreen()) * vector;
    case CoordType::kScreen:
      ASSERT(type_out == CoordType::kWorld);
      return glm::mat2(ScreenToWorld()) * vector;
  }
}

float Camera::ConvertDistance(float distance, DistanceType type_in,
                              DistanceType type_out) const {
  if (type_in == type_out) return distance;

  float screen_distance = 0;
  switch (type_in) {
    case DistanceType::kWorld:
      screen_distance = distance / ScaleFactor();
      break;
    case DistanceType::kScreen:
      screen_distance = distance;
      break;
    case DistanceType::kCm:
      screen_distance = distance * GetPPI() / kCmPerInch;
      break;
    case DistanceType::kDp:
      if (distance == 0) {
        screen_distance = 0;
      } else {
        screen_distance = std::copysign(
            std::max(std::round(std::abs(distance * GetPPI() /
                                         kDeviceIndependentPixelsPerInch)),
                     1.0f),
            distance);
      }
      break;
  }

  float output_distance = 0;
  switch (type_out) {
    case DistanceType::kWorld:
      output_distance = screen_distance * ScaleFactor();
      break;
    case DistanceType::kScreen:
      output_distance = screen_distance;
      break;
    case DistanceType::kCm:
      output_distance = screen_distance * kCmPerInch / GetPPI();
      break;
    case DistanceType::kDp:
      output_distance =
          screen_distance * kDeviceIndependentPixelsPerInch / GetPPI();
      break;
  }

  return output_distance;
}

glm::vec2 Camera::WorldCenter() const {
  return ConvertPosition(.5f * glm::vec2(screen_dim_), CoordType::kScreen,
                         CoordType::kWorld);
}

glm::vec2 Camera::WorldDim() const {
  return ScaleFactor() * glm::vec2(ScreenDim());
}

float Camera::WorldRotation() const {
  return matrix_utils::GetRotationComponent(ScreenToWorld());
}

RotRect Camera::WorldRotRect() const {
  return RotRect(WorldCenter(), WorldDim(), WorldRotation());
}

void Camera::UnFlipWorldToDevice() { RecalcMatrices(); }

void Camera::FlipWorldToDevice(float rotation_deg) {
  glm::mat4 rot =
      glm::rotate(glm::mat4{1}, glm::radians(rotation_deg), glm::vec3{0, 0, 1});
  glm::mat4 flip = glm::scale(glm::mat4{1}, glm::vec3(1, -1, 1));
  world_to_device_ = flip * rot * world_to_device_;
}

void Camera::RotateWorldToDevice(float rotation_deg) {
  glm::mat4 rot =
      glm::rotate(glm::mat4{1}, glm::radians(rotation_deg), glm::vec3{0, 0, 1});
  world_to_device_ = rot * world_to_device_;
}

std::string Camera::ToString() const {
  return Substitute("camera $0 (window: $0)", AddressStr(this), WorldWindow());
}

Camera& Camera::operator=(const Camera& other) {
  screen_dim_ = other.screen_dim_;
  ppi_ = other.ppi_;
  screen_to_device_ = other.screen_to_device_;
  world_to_screen_ = other.world_to_screen_;
  screen_to_world_ = other.screen_to_world_;
  world_to_device_ = other.world_to_device_;
  screen_rotation_deg_ = other.screen_rotation_deg_;
  return *this;
}

bool Camera::ScreenWorldAndPPIApproximatelyEq(
    const Camera& cam1, const Camera& cam2,
    const float screen_size_relative_tolerance,
    const float world_window_relative_tolerance,
    const float ppi_absolute_epsilon) {
  return
      // World windows.
      geometry::Equivalent(cam1.WorldWindow(), cam2.WorldWindow(),
                           world_window_relative_tolerance) &&
      // PPI.
      std::abs(cam1.GetPPI() - cam2.GetPPI()) <= ppi_absolute_epsilon &&
      // Screen dimensions last, to avoid temporary object construction when
      // possible.
      geometry::Equivalent(Rect(0, 0, cam1.ScreenDim().x, cam1.ScreenDim().y),
                           Rect(0, 0, cam2.ScreenDim().x, cam2.ScreenDim().y),
                           screen_size_relative_tolerance);
}

Status Camera::IsValidViewport(const proto::Viewport& proto) {
  if (!BoundsCheckIncInc(proto.ppi(), 1, kMaxPPI))
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "PPI must lie in the range [1, $0].", kMaxPPI);
  if (!BoundsCheckIncInc(proto.width(), 1, kMaxViewportSize) ||
      !BoundsCheckIncInc(proto.height(), 1, kMaxViewportSize))
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Viewport dimensions must lie in the range [1, $0].",
                       kMaxViewportSize);
  return OkStatus();
}

Status Camera::IsValidCameraPosition(const proto::CameraPosition& proto) {
  if (proto.world_width() <= 0 && proto.world_height() <= 0)
    return ErrorStatus(
        "At least one of width and height must be greater than zero.");
  return OkStatus();
}

Status Camera::IsValidCameraSettings(const proto::CameraSettings& proto) {
  INK_RETURN_UNLESS(IsValidViewport(proto.viewport()));
  return IsValidCameraPosition(proto.position());
}

void Camera::WriteViewportProto(proto::Viewport* proto, const Camera& cam) {
  glm::ivec2 screen_dim = cam.ScreenDim();
  proto->set_width(screen_dim.x);
  proto->set_height(screen_dim.y);
  proto->set_ppi(cam.GetPPI());
}

void Camera::WriteCameraPositionProto(proto::CameraPosition* proto,
                                      const Camera& cam) {
  glm::vec2 world_center = cam.WorldCenter();
  glm::vec2 world_dim = cam.WorldDim();
  proto->mutable_world_center()->set_x(world_center.x);
  proto->mutable_world_center()->set_y(world_center.y);
  proto->set_world_width(world_dim.x);
  proto->set_world_height(world_dim.y);
}

void Camera::WriteToProto(proto::CameraSettings* proto, const Camera& cam) {
  WriteViewportProto(proto->mutable_viewport(), cam);
  WriteCameraPositionProto(proto->mutable_position(), cam);
}

Status Camera::ReadViewportProto(const proto::Viewport& proto, Camera* cam) {
  INK_RETURN_UNLESS(IsValidViewport(proto));
  ReadViewportWithoutValidating(proto, cam);
  return OkStatus();
}

Status Camera::ReadCameraPositionProto(const proto::CameraPosition& proto,
                                       Camera* cam) {
  INK_RETURN_UNLESS(IsValidCameraPosition(proto));
  ReadCameraPositionWithoutValidating(proto, cam);
  return OkStatus();
}

Status Camera::ReadFromProto(const proto::CameraSettings& proto, Camera* cam) {
  INK_RETURN_UNLESS(IsValidCameraSettings(proto));
  ReadViewportWithoutValidating(proto.viewport(), cam);
  ReadCameraPositionWithoutValidating(proto.position(), cam);
  return OkStatus();
}

}  // namespace ink
