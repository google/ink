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

#include "ink/geometry/internal/jni/intersection_native.h"

#include <cstdint>

#include "ink/geometry/affine_transform.h"
#include "ink/geometry/angle.h"
#include "ink/geometry/internal/jni/partitioned_mesh_native_helper.h"
#include "ink/geometry/intersects.h"
#include "ink/geometry/point.h"
#include "ink/geometry/quad.h"
#include "ink/geometry/rect.h"
#include "ink/geometry/segment.h"
#include "ink/geometry/triangle.h"

using ::ink::AffineTransform;
using ::ink::Angle;
using ::ink::Intersects;
using ::ink::Point;
using ::ink::Quad;
using ::ink::Rect;
using ::ink::Segment;
using ::ink::Triangle;
using ::ink::native::CastToPartitionedMesh;

extern "C" {

bool IntersectionNative_vecSegmentIntersects(float vec_x, float vec_y,
                                             float segment_start_x,
                                             float segment_start_y,
                                             float segment_end_x,
                                             float segment_end_y) {
  return Intersects(Point{vec_x, vec_y},
                    Segment{{segment_start_x, segment_start_y},
                            {segment_end_x, segment_end_y}});
}

bool IntersectionNative_vecTriangleIntersects(
    float vec_x, float vec_y, float triangle_p0_x, float triangle_p0_y,
    float triangle_p1_x, float triangle_p1_y, float triangle_p2_x,
    float triangle_p2_y) {
  return Intersects(Point{vec_x, vec_y},
                    Triangle{{triangle_p0_x, triangle_p0_y},
                             {triangle_p1_x, triangle_p1_y},
                             {triangle_p2_x, triangle_p2_y}});
}

bool IntersectionNative_vecBoxIntersects(float vec_x, float vec_y,
                                         float box_x_min, float box_y_min,
                                         float box_x_max, float box_y_max) {
  return Intersects(
      Point{vec_x, vec_y},
      Rect::FromTwoPoints({box_x_min, box_y_min}, {box_x_max, box_y_max}));
}

bool IntersectionNative_vecParallelogramIntersects(
    float vec_x, float vec_y, float parallelogram_center_x,
    float parallelogram_center_y, float parallelogram_width,
    float parallelogram_height, float parallelogram_angle_radian,
    float parallelogram_shear_factor) {
  return Intersects(
      Point{vec_x, vec_y},
      Quad::FromCenterDimensionsRotationAndSkew(
          {parallelogram_center_x, parallelogram_center_y}, parallelogram_width,
          parallelogram_height, Angle::Radians(parallelogram_angle_radian),
          parallelogram_shear_factor));
}

bool IntersectionNative_segmentSegmentIntersects(
    float segment_1_start_x, float segment_1_start_y, float segment_1_end_x,
    float segment_1_end_y, float segment_2_start_x, float segment_2_start_y,
    float segment_2_end_x, float segment_2_end_y) {
  return Intersects(Segment{{segment_1_start_x, segment_1_start_y},
                            {segment_1_end_x, segment_1_end_y}},
                    Segment{{segment_2_start_x, segment_2_start_y},
                            {segment_2_end_x, segment_2_end_y}});
}

bool IntersectionNative_segmentTriangleIntersects(
    float segment_start_x, float segment_start_y, float segment_end_x,
    float segment_end_y, float triangle_p0_x, float triangle_p0_y,
    float triangle_p1_x, float triangle_p1_y, float triangle_p2_x,
    float triangle_p2_y) {
  return Intersects(Segment{{segment_start_x, segment_start_y},
                            {segment_end_x, segment_end_y}},
                    Triangle{{triangle_p0_x, triangle_p0_y},
                             {triangle_p1_x, triangle_p1_y},
                             {triangle_p2_x, triangle_p2_y}});
}

bool IntersectionNative_segmentBoxIntersects(float segment_start_x,
                                             float segment_start_y,
                                             float segment_end_x,
                                             float segment_end_y,
                                             float box_x_min, float box_y_min,
                                             float box_x_max, float box_y_max) {
  return Intersects(
      Segment{{segment_start_x, segment_start_y},
              {segment_end_x, segment_end_y}},
      Rect::FromTwoPoints({box_x_min, box_y_min}, {box_x_max, box_y_max}));
}

bool IntersectionNative_segmentParallelogramIntersects(
    float segment_start_x, float segment_start_y, float segment_end_x,
    float segment_end_y, float parallelogram_center_x,
    float parallelogram_center_y, float parallelogram_width,
    float parallelogram_height, float parallelogram_angle_radian,
    float parallelogram_shear_factor) {
  return Intersects(
      Segment{{segment_start_x, segment_start_y},
              {segment_end_x, segment_end_y}},
      Quad::FromCenterDimensionsRotationAndSkew(
          {parallelogram_center_x, parallelogram_center_y}, parallelogram_width,
          parallelogram_height, Angle::Radians(parallelogram_angle_radian),
          parallelogram_shear_factor));
}

bool IntersectionNative_triangleTriangleIntersects(
    float triangle_1_p0_x, float triangle_1_p0_y, float triangle_1_p1_x,
    float triangle_1_p1_y, float triangle_1_p2_x, float triangle_1_p2_y,
    float triangle_2_p0_x, float triangle_2_p0_y, float triangle_2_p1_x,
    float triangle_2_p1_y, float triangle_2_p2_x, float triangle_2_p2_y) {
  return Intersects(Triangle{{triangle_1_p0_x, triangle_1_p0_y},
                             {triangle_1_p1_x, triangle_1_p1_y},
                             {triangle_1_p2_x, triangle_1_p2_y}},
                    Triangle{{triangle_2_p0_x, triangle_2_p0_y},
                             {triangle_2_p1_x, triangle_2_p1_y},
                             {triangle_2_p2_x, triangle_2_p2_y}});
}

bool IntersectionNative_triangleBoxIntersects(
    float triangle_p0_x, float triangle_p0_y, float triangle_p1_x,
    float triangle_p1_y, float triangle_p2_x, float triangle_p2_y,
    float box_x_min, float box_y_min, float box_x_max, float box_y_max) {
  return Intersects(
      Triangle{{triangle_p0_x, triangle_p0_y},
               {triangle_p1_x, triangle_p1_y},
               {triangle_p2_x, triangle_p2_y}},
      Rect::FromTwoPoints({box_x_min, box_y_min}, {box_x_max, box_y_max}));
}

bool IntersectionNative_triangleParallelogramIntersects(
    float triangle_p0_x, float triangle_p0_y, float triangle_p1_x,
    float triangle_p1_y, float triangle_p2_x, float triangle_p2_y,
    float parallelogram_center_x, float parallelogram_center_y,
    float parallelogram_width, float parallelogram_height,
    float parallelogram_angle_radian, float parallelogram_shear_factor) {
  return Intersects(
      Triangle{{triangle_p0_x, triangle_p0_y},
               {triangle_p1_x, triangle_p1_y},
               {triangle_p2_x, triangle_p2_y}},
      Quad::FromCenterDimensionsRotationAndSkew(
          {parallelogram_center_x, parallelogram_center_y}, parallelogram_width,
          parallelogram_height, Angle::Radians(parallelogram_angle_radian),
          parallelogram_shear_factor));
}

bool IntersectionNative_boxBoxIntersects(float box_1_x_min, float box_1_y_min,
                                         float box_1_x_max, float box_1_y_max,
                                         float box_2_x_min, float box_2_y_min,
                                         float box_2_x_max, float box_2_y_max) {
  return Intersects(Rect::FromTwoPoints({box_1_x_min, box_1_y_min},
                                        {box_1_x_max, box_1_y_max}),
                    Rect::FromTwoPoints({box_2_x_min, box_2_y_min},
                                        {box_2_x_max, box_2_y_max}));
}

bool IntersectionNative_boxParallelogramIntersects(
    float box_x_min, float box_y_min, float box_x_max, float box_y_max,
    float parallelogram_center_x, float parallelogram_center_y,
    float parallelogram_width, float parallelogram_height,
    float parallelogram_angle_radian, float parallelogram_shear_factor) {
  return Intersects(
      Rect::FromTwoPoints({box_x_min, box_y_min}, {box_x_max, box_y_max}),
      Quad::FromCenterDimensionsRotationAndSkew(
          {parallelogram_center_x, parallelogram_center_y}, parallelogram_width,
          parallelogram_height, Angle::Radians(parallelogram_angle_radian),
          parallelogram_shear_factor));
}

bool IntersectionNative_parallelogramParallelogramIntersects(
    float parallelogram_1_center_x, float parallelogram_1_center_y,
    float parallelogram_1_width, float parallelogram_1_height,
    float parallelogram_1_angle_in_radian, float parallelogram_1_shear_factor,
    float parallelogram_2_center_x, float parallelogram_2_center_y,
    float parallelogram_2_width, float parallelogram_2_height,
    float parallelogram_2_angle_in_radian, float parallelogram_2_shear_factor) {
  return Intersects(Quad::FromCenterDimensionsRotationAndSkew(
                        {parallelogram_1_center_x, parallelogram_1_center_y},
                        parallelogram_1_width, parallelogram_1_height,
                        Angle::Radians(parallelogram_1_angle_in_radian),
                        parallelogram_1_shear_factor),
                    Quad::FromCenterDimensionsRotationAndSkew(
                        {parallelogram_2_center_x, parallelogram_2_center_y},
                        parallelogram_2_width, parallelogram_2_height,
                        Angle::Radians(parallelogram_2_angle_in_radian),
                        parallelogram_2_shear_factor));
}

bool IntersectionNative_partitionedMeshVecIntersects(
    int64_t partitioned_mesh_ptr, float vec_x, float vec_y, float a, float b,
    float c, float d, float e, float f) {
  return Intersects(Point{vec_x, vec_y},
                    CastToPartitionedMesh(partitioned_mesh_ptr),
                    AffineTransform(a, b, c, d, e, f));
}

bool IntersectionNative_partitionedMeshSegmentIntersects(
    int64_t partitioned_mesh_ptr, float segment_start_x, float segment_start_y,
    float segment_end_x, float segment_end_y, float a, float b, float c,
    float d, float e, float f) {
  return Intersects(Segment{{segment_start_x, segment_start_y},
                            {segment_end_x, segment_end_y}},
                    CastToPartitionedMesh(partitioned_mesh_ptr),
                    AffineTransform(a, b, c, d, e, f));
}

bool IntersectionNative_partitionedMeshTriangleIntersects(
    int64_t partitioned_mesh_ptr, float triangle_p0_x, float triangle_p0_y,
    float triangle_p1_x, float triangle_p1_y, float triangle_p2_x,
    float triangle_p2_y, float a, float b, float c, float d, float e, float f) {
  return Intersects(Triangle{{triangle_p0_x, triangle_p0_y},
                             {triangle_p1_x, triangle_p1_y},
                             {triangle_p2_x, triangle_p2_y}},
                    CastToPartitionedMesh(partitioned_mesh_ptr),
                    AffineTransform(a, b, c, d, e, f));
}

bool IntersectionNative_partitionedMeshBoxIntersects(
    int64_t partitioned_mesh_ptr, float box_x_min, float box_y_min,
    float box_x_max, float box_y_max, float a, float b, float c, float d,
    float e, float f) {
  return Intersects(
      Rect::FromTwoPoints({box_x_min, box_y_min}, {box_x_max, box_y_max}),
      CastToPartitionedMesh(partitioned_mesh_ptr),
      AffineTransform(a, b, c, d, e, f));
}

bool IntersectionNative_partitionedMeshParallelogramIntersects(
    int64_t partitioned_mesh_ptr, float parallelogram_center_x,
    float parallelogram_center_y, float parallelogram_width,
    float parallelogram_height, float parallelogram_angle_radian,
    float parallelogram_shear_factor, float a, float b, float c, float d,
    float e, float f) {
  return Intersects(
      Quad::FromCenterDimensionsRotationAndSkew(
          {parallelogram_center_x, parallelogram_center_y}, parallelogram_width,
          parallelogram_height, Angle::Radians(parallelogram_angle_radian),
          parallelogram_shear_factor),
      CastToPartitionedMesh(partitioned_mesh_ptr),
      AffineTransform(a, b, c, d, e, f));
}

bool IntersectionNative_partitionedMeshPartitionedMeshIntersects(
    int64_t this_partitioned_mesh_ptr, int64_t other_partitioned_mesh_ptr,
    float this_a, float this_b, float this_c, float this_d, float this_e,
    float this_f, float other_a, float other_b, float other_c, float other_d,
    float other_e, float other_f) {
  return Intersects(
      CastToPartitionedMesh(this_partitioned_mesh_ptr),
      AffineTransform(this_a, this_b, this_c, this_d, this_e, this_f),
      CastToPartitionedMesh(other_partitioned_mesh_ptr),
      AffineTransform(other_a, other_b, other_c, other_d, other_e, other_f));
}

}  // extern "C"
