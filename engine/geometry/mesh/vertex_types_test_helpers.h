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

#ifndef INK_ENGINE_GEOMETRY_MESH_VERTEX_TYPES_TEST_HELPERS_H_
#define INK_ENGINE_GEOMETRY_MESH_VERTEX_TYPES_TEST_HELPERS_H_

#include "testing/base/public/gunit.h"
#include "ink/engine/geometry/mesh/vertex_types.h"
namespace ink {

inline int GetFloatCount(const PackedVertList& packed_vert_list) {
  int data_size_bytes =
      packed_vert_list.size() * packed_vert_list.VertexSizeBytes();
  int remainder = data_size_bytes % sizeof(float);
  EXPECT_EQ(0, remainder);
  return data_size_bytes / sizeof(float);
}

inline void CheckPackedDataPtr(const float* expected_data,
                               const PackedVertList& actual,
                               int expected_floats_to_check) {
  ASSERT_EQ(expected_floats_to_check, GetFloatCount(actual));
  auto actual_data = static_cast<const float*>(actual.Ptr());
  for (int i = 0; i < expected_floats_to_check; i++) {
    EXPECT_EQ(expected_data[i], actual_data[i]) << "Testing " << i
                                                << "th float";
  }
}

inline void CheckPackedVertDataEqual(const PackedVertList& expected,
                                     const PackedVertList& actual) {
  ASSERT_EQ(expected.GetFormat(), actual.GetFormat());
  ASSERT_EQ(expected.size(), actual.size());
  ASSERT_EQ(expected.VertexSizeBytes(), actual.VertexSizeBytes());
  auto expected_data = static_cast<const float*>(expected.Ptr());
  int expected_data_size_floats = GetFloatCount(expected);
  CheckPackedDataPtr(expected_data, actual, expected_data_size_floats);
}

}  // namespace ink
#endif  // INK_ENGINE_GEOMETRY_MESH_VERTEX_TYPES_TEST_HELPERS_H_
