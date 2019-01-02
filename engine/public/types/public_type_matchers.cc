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

#include "ink/engine/public/types/public_type_matchers.h"

#include <memory>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/geometry/mesh/mesh_test_helpers.h"
#include "ink/engine/scene/data/common/mesh_serializer_provider.h"
#include "ink/engine/scene/data/common/processed_element.h"
#include "ink/engine/scene/data/common/stroke.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

Mesh getFlatMeshFromBundle(const ink::proto::ElementBundle bundle) {
  ink::Stroke s;
  EXPECT_OK(ink::util::ReadFromProto(bundle, &s));
  Mesh m;
  EXPECT_OK(s.GetMesh(*mesh::ReaderFor(s), 0, &m));
  return FlattenObjectMatrix(m);
}
}  // namespace ink
