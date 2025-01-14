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
#include "ink/geometry/partitioned_mesh.h"
#include "ink/geometry/point.h"
#include "ink/geometry/tessellator.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/strokes/input/stroke_input_batch.h"

namespace {

using ::ink::Mesh;
using ::ink::PartitionedMesh;
using ::ink::Point;
using ::ink::StrokeInputBatch;

}  // namespace

extern "C" {

JNI_METHOD(geometry, MeshCreationNative, jlong,
           nativeCreateClosedShapeFromStokeInputBatch)
(JNIEnv* env, jclass clazz, jlong stroke_input_batch_native_pointer,
 jclass immutable_vec_class) {
  const auto* input = reinterpret_cast<const StrokeInputBatch*>(
      stroke_input_batch_native_pointer);

  std::vector<Point> points;
  points.reserve(input->Size());
  for (size_t i = 0; i < input->Size(); ++i) {
    points.push_back(input->Get(i).position);
  }

  std::vector<Point> processed_points =
      ink::geometry_internal::CreateClosedShape(points);

  absl::StatusOr<Mesh> mesh = ink::CreateMeshFromPolyline(processed_points);

  absl::StatusOr<PartitionedMesh> partitioned_mesh =
      PartitionedMesh::FromMeshes(absl::MakeSpan(&mesh.value(), 1));
  auto partitioned_mesh_ptr =
      std::make_unique<PartitionedMesh>(partitioned_mesh.value());
  return reinterpret_cast<jlong>(partitioned_mesh_ptr.release());
}

}  // extern "C
