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

#ifndef INK_ENGINE_CAMERA_CAMERA_H_
#define INK_ENGINE_CAMERA_CAMERA_H_

#include <memory>
#include <string>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/algorithms/envelope.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/geometry/primitives/rot_rect.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/service/dependencies.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/proto/sengine_portable_proto.pb.h"

namespace ink {

enum class CoordType {
  // World coordinates describe the position of objects in the scene.
  kWorld,
  // Screen coordinates describe the position of objects on the screen, in
  // pixels. The bottom-left corner of the viewport is (0, 0), and the top-right
  // corner is (width, height). Note that while the position of physical pixels
  // on the screen are integer values in [0, width]×[0, height], screen
  // coordinates in general are floating point numbers, and are not bounded by
  // the physical screen's dimensions.
  kScreen
};

enum class DistanceType {
  // Distance in world units.
  kWorld,
  // Distance in screen pixels. Similarly to CoordType::kScreen, screen
  // distances are not necessarily integer values
  kScreen,
  // Distance in the real-world, in centimeters.
  kCm,
  // Distance in device-independent pixels (go/wiki/Device-independent_pixel),
  // which we define to be 160 dots-per-inch (the same as Android).
  // Warning: The conversion from device-independent pixels another distance
  // type is not a continuous mapping. To perform the conversion, we first
  // convert to screen pixels using the formula:
  //     s = sign(d)⋅max(round(abs(d⋅ppi/160), 1))
  // where s is the distance in screen pixels, d is the distance in
  // device-independent pixels, and ppi is the pixel-per-inch ratio of the
  // screen. We can then convert from screen pixels to the desired distance
  // type. Similarly, when converting from another distance type to
  // device-independent pixels, we first convert to screen pixels, and then
  // convert to device-independent pixels using the formula:
  //     d = s⋅160/ppi
  // Note that, because of the rounding, converting back-and-forth between
  // device-independent pixels and another distance type will not round-trip.
  kDp
};

// The camera encapsulates the dimensions and pixel density of the viewport
// (the portion of the screen that the engine is drawn to), and the area of the
// world that is visible in the viewport. It is responsible for the matrices
// defining the transformations between the screen-, world-, and normalized
// device-coordinates, and, from that, unit conversions.
//
// The engine has a root camera, which determines what is actually drawn to the
// screen, but there are also many other instances, used for things such as
// animation, prediction, and drawing to buffers. For the most part, the code
// that depends on the camera shouldn't know or care which instance it's
// operating on. If you want to influence the root camera, you should use the
// CameraController.
class Camera {
 public:
  // Constructs a camera with a 1000x1000 viewport at 160 DPI, showing the
  // rectangle (-500, -500) to (500, 500) in world coordinates.
  Camera();

  // The dimensions of the viewport. When the screen dimensions are changed, the
  // camera remains centered on the same world position, and maintains the scale
  // between screen and world coordinates.
  glm::ivec2 ScreenDim() const { return screen_dim_; }
  void SetScreenDim(glm::ivec2 size);

  // The pixel density of the screen, in pixels-per-inch.
  float GetPPI() const { return ppi_; }
  void SetPPI(float ppi) {
    ASSERT(ppi > 0);
    ppi_ = ppi;
  }

  // Set the screen rotation relative to the canvas.  Must be a multiple of 90.
  //
  // This is maintained through calls to SetPosition, as the screen rotation is
  // independent of any camera manipulations.  This is set from the various
  // platform APIs that report screen rotation, e.g.
  // window.screen.orientation.onchange.
  int ScreenRotation() const { return screen_rotation_deg_; }
  void SetScreenRotation(int rotation_deg) {
    ASSERT(rotation_deg % 90 == 0);
    screen_rotation_deg_ = rotation_deg;
  }

  // Transformation matrices between world, screen, and normalized device
  // coordinates.
  // In screen coordinates, the window will always cover the rectangle from
  // (0, 0) to ScreenDim(). Likewise, in normalized device coordinates, the
  // window will cover the rect from (-1, -1) to (1, 1).
  const glm::mat4& WorldToScreen() const { return world_to_screen_; }
  const glm::mat4& ScreenToWorld() const { return screen_to_world_; }
  const glm::mat4& ScreenToDevice() const { return screen_to_device_; }
  const glm::mat4& WorldToDevice() const { return world_to_device_; }

  // Converts a position from one coordinate type to another.
  glm::vec2 ConvertPosition(glm::vec2 position, CoordType type_in,
                            CoordType type_out) const;

  // Converts the vector difference between two points from one coordinate type
  // to another, ignoring translation between origins of the coordinate systems.
  // This is equivalent to the statement:
  //     ConvertPosition(vector, type_in, type_out) -
  //         ConvertPosition({0, 0}, type_in, type_out);
  glm::vec2 ConvertVector(glm::vec2 vector, CoordType type_in,
                          CoordType type_out) const;

  // Converts a distance from one distance type to another, preserving the sign.
  float ConvertDistance(float distance, DistanceType type_in,
                        DistanceType type_out) const;

  // The center of the screen, in world-coordinates.
  glm::vec2 WorldCenter() const;

  // The dimensions of the window, in world-coordinates.
  glm::vec2 WorldDim() const;

  // The angle, in radians, from the x-axis of the screen to the x-axis of the
  // world, measured counter-clockwise.
  // Note: The return value will lie in the interval (-π, π].
  float WorldRotation() const;

  // The rotated rect corresponding to the camera's current view of the world.
  RotRect WorldRotRect() const;

  // The length of a pixel in world units.
  float ScaleFactor() const;

  // Set the position of the camera, such that it is centered on world_center,
  // the visible area's dimensions are world_dim, and the counter-clockwise
  // angle from the x-axis of the screen to the x-axis of the world is
  // rotation_radians.
  // Note: If the aspect ratio of world_dim is not consistent with the viewport,
  // the width or height will be increased to match the viewport's aspect ratio.
  void SetPosition(glm::vec2 world_center, glm::vec2 world_dim,
                   float rotation_radians);

  void SetWorldWindow(Rect r) { SetPosition(r.Center(), r.Dim(), 0); }
  Rect WorldWindow() const { return geometry::Envelope(WorldRotRect()); }

  // The following three methods (Translate(), Scale(), and Rotate()) provide a
  // means to change the camera position relative to its current position. You
  // can think of these as applying a the appropriate affine transform to the
  // (non-axis-aligned) window rectangle.

  // Translates the camera by the given vector, in world-coordinates.
  void Translate(glm::vec2 world_translation);

  // Scales the world dimensions of the camera by the given factor, maintaining
  // the screen position of world_scale_center. This creates the effect of
  // zooming centered on world_scale_center, where a factor less than one gives
  // the appearance of zooming in, and a factor greater than one gives the
  // appearance of zooming out.
  // Note: If the resulting window would cause in precision loss (see
  // WithinPrecisionBounds()), this becomes a no-op.
  void Scale(float factor, glm::vec2 world_scale_center);

  // Rotates the window counter-clockwise by angle_radians about
  // world_rotation_center.
  void Rotate(float angle_radians, glm::vec2 world_rotation_center);

  // Returns true iff, given the limitations of floating-point precision, we can
  // represent each pixel as a distinct world position and each visible integer
  // world position as a distinct floating-point screen position.
  // Note that "distinct" does not necessarily imply an integer position, just
  // that the floating-point values are not exactly equal.
  bool WithinPrecisionBounds() const;

  // Calculates the coverage of an object with the given linear width relative
  // to the current world window width.
  float Coverage(const float width) const;

  std::string ToString() const;

  // Optionally rotates and then flips camera so draw calls immediately
  // following will produce a (rotated and) upsidedown image.  This is used for
  // generating images in coordinate systems where the origin is not in the
  // lower left corner, (e.g., for exporting images, compatibility with hardware
  // overlays).
  //
  // The camera is only guaranteed to remain flipped if only const methods are
  // called.  Any other modification to the camera will result in RecalcMatrices
  // overwriting the flip.  FlipWorldToDevice directly manipulates the
  // WorldToDevice matrix, which is otherwise always derived from the
  // ScreenToWorld and ScreenToDevice matrices in RecalcMatrices.
  void FlipWorldToDevice(float rotation_deg = 0.0f);

  // Un-flip the camera, which calls RecalcMatrices to reset the WorldToDevice
  // matrix to be consistent with the WorldToScreen and ScreenToDevice matrices.
  //
  // If camera is not flipped, then RecalcMatrices has no effect.
  void UnFlipWorldToDevice();

  void RotateWorldToDevice(float rotation_deg);

  // Returns true iff the WorldToDevice transform of this is exactly
  // equal to that of other.
  bool operator==(const Camera& other) const {
    return WorldToDevice() == other.WorldToDevice();
  }

  // Exact inverse of operator==.
  bool operator!=(const Camera& other) const { return !operator==(other); }

  Camera& operator=(const Camera& other);

  // Returns true iff cam1 and cam2 have the same screen dimensions,
  // world window, and PPI, to the degrees of tolerance specified.
  //
  // Screen dimensions are considered equivalent if they form
  // rectangles whose overlap accounts for the area of each
  // screen. screen_size_tolerance defines the maximum fraction of
  // each rectangle's area that may go unaccounted for.
  //
  // World windows are considered equivalent on a similar basis, using
  // the area of their intersection.
  //
  // PPI is compared absolutely.
  static bool ScreenWorldAndPPIApproximatelyEq(
      const Camera& cam1, const Camera& cam2,
      float screen_size_relative_tolerance = 0.001,
      float world_window_relative_tolerance = 0.001,
      float ppi_absolute_epsilon = 0.01);

  // The CameraSettings proto, which is comprised of a Viewport proto and a
  // CameraPosition proto, describes a complete camera. However, because the
  // viewport and position can be varied semi-independently (subject to the
  // contraints of SetScreenDim() and SetPosition()), we provide validation
  // and serialization of the Viewport and CameraPosition protos as well.

  // Returns ok-status iff the proto has valid values that can be used to
  // create a logically coherent camera. Note that these do not validate
  // floating-point precision bounds.
  static Status IsValidViewport(const proto::Viewport& proto);
  static Status IsValidCameraPosition(const proto::CameraPosition& proto);
  static Status IsValidCameraSettings(const proto::CameraSettings& proto);

  // Writes the relevant portion of the camera's state into the proto.
  static void WriteViewportProto(proto::Viewport* proto, const Camera& cam);
  static void WriteCameraPositionProto(proto::CameraPosition* proto,
                                       const Camera& cam);
  static void WriteToProto(proto::CameraSettings* proto, const Camera& cam);

  // Reads the relevant portion of the camera's state from the proto, returning
  // ok-status on success. On failure, the given camera will not be modified.
  // Note that these set the camera's state via SetScreenDim() and
  // SetPosition(), so the same constraints and behavior apply.
  static Status ReadViewportProto(const proto::Viewport& proto, Camera* cam);
  static Status ReadCameraPositionProto(const proto::CameraPosition& proto,
                                        Camera* cam);
  static Status ReadFromProto(const proto::CameraSettings& proto, Camera* cam);

 private:
  // Recomputes the cached world_to_screen_ and world_to_device_ matrices from
  // the screen_to_world_ and screen_to_device_ matrices.
  void RecalcMatrices();

  // Returns true iff the given screen dimensions and screen-to-world transform
  // could be used to define a camera that is safe for the floating-point
  // precision (see WithinPrecisionBounds()).
  static bool IsTransformSafeForPrecision(glm::ivec2 screen_dim,
                                          const glm::mat4& screen_to_world);

  float ppi_ = 160;
  glm::ivec2 screen_dim_{0, 0};

  glm::mat4 screen_to_device_{1};
  glm::mat4 world_to_screen_{1};
  glm::mat4 screen_to_world_{1};
  glm::mat4 world_to_device_{1};

  int screen_rotation_deg_ = 0;  // Degrees.
};

}  // namespace ink
#endif  // INK_ENGINE_CAMERA_CAMERA_H_
