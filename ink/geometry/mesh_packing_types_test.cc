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

#include "ink/geometry/mesh_packing_types.h"

#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"

namespace ink {
namespace {

TEST(MeshPackingTypesTest, StringifyMeshAttributeCodingParams) {
  EXPECT_EQ(absl::StrCat(MeshAttributeCodingParams{}), "[]");
  EXPECT_EQ(
      absl::StrCat(MeshAttributeCodingParams{{{.offset = 0, .scale = 1}}}),
      "[{offset=0, scale=1}]");
  EXPECT_EQ(absl::StrCat(MeshAttributeCodingParams{
                {{.offset = 0, .scale = 1}, {.offset = 2, .scale = 3}}}),
            "[{offset=0, scale=1}, {offset=2, scale=3}]");
}

}  // namespace
}  // namespace ink
