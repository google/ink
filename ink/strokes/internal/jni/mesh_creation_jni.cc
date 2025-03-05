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

#include <cstddef>
#include <memory>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "ink/geometry/internal/polyline_processing.h"
#include "ink/geometry/mesh.h"
#include "ink/geometry/mesh_format.h"
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/tessellator.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace {

using ::ink::Mesh;
using ::ink::PartitionedMesh;
using ::ink::Point;
using ::ink::StrokeInputBatch;

}  // namespace

extern "C" {

JNI_METHOD(strokes, MeshCreationNative, jlong,
           createClosedShapeFromStrokeInputBatch)
(JNIEnv* env, jobject object, jlong stroke_input_batch_native_pointer) {
  const auto* input = reinterpret_cast<const StrokeInputBatch*>(
      stroke_input_batch_native_pointer);

  // If the input is empty then this will return an empty PartitionedMesh with
  // no location and no area. This will not intersect with anything if used for
  // hit testing.
  if (input->IsEmpty()) {
    return reinterpret_cast<jlong>(
        std::make_unique<PartitionedMesh>(PartitionedMesh()).release());
  }

  std::vector<Point> points;
  points.reserve(input->Size());
  for (size_t i = 0; i < input->Size(); ++i) {
    points.push_back(input->Get(i).position);
  }

  std::vector<Point> processed_points =
      ink::geometry_internal::CreateClosedShape(points);

  absl::StatusOr<Mesh> mesh;
  // If there are fewer than 3 points the tessellator can't be used to create a
  // mesh. Instead, the mesh is created with a single triangle that has repeated
  // and overlapping points. This effectively creates a point-like or
  // segment-like mesh. The resulting mesh will have an area of 0 but can still
  // be used for hit testing via intersection.
  if (processed_points.size() < 3) {
    const size_t size = processed_points.size();
    std::vector<float> x_values;
    std::vector<float> y_values;
    x_values.reserve(3);
    y_values.reserve(3);
    for (int i = 0; i < 3; ++i) {
      // If there are 2 points remaining then the first point will appear twice.
      // If there is only 1 point all 3 points will be the same.
      x_values.push_back(processed_points[i % size].x);
      y_values.push_back(processed_points[i % size].y);
    }
    mesh = Mesh::Create(ink::MeshFormat(), {x_values, y_values}, {0, 1, 2});
  } else {
    mesh = ink::CreateMeshFromPolyline(processed_points);
  }
  if (!ink::jni::CheckOkOrThrow(env, mesh.status())) {
    return 0;
  }

  absl::StatusOr<PartitionedMesh> partitioned_mesh =
      PartitionedMesh::FromMeshes(absl::MakeSpan(&mesh.value(), 1));
  if (!ink::jni::CheckOkOrThrow(env, partitioned_mesh.status())) {
    return 0;
  }
  return reinterpret_cast<jlong>(new PartitionedMesh(*partitioned_mesh));
}

}  // extern "C
