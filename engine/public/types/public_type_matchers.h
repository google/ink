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

#ifndef INK_ENGINE_PUBLIC_TYPES_PUBLIC_TYPE_MATCHERS_H_
#define INK_ENGINE_PUBLIC_TYPES_PUBLIC_TYPE_MATCHERS_H_

#include "testing/base/public/gmock.h"
#include "testing/base/public/gunit.h"

#include "ink/engine/geometry/mesh/mesh.h"
#include "ink/engine/geometry/mesh/type_test_helpers.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/proto/serialize.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

// The matchers and utilities provided in this file are useful for verifying
// behavior that the public api of sketchology (Host) produces the
// expected results.  If a test using these matchers fails and you have
// difficulty debugging, see README.md.

/////////////////////// Element Bundle Matchers ///////////////////////////////

// Helper to generate a mesh with object matrix of the identity from an element
// bundle (transform in bundle is applied to coordinates of mesh).
Mesh getFlatMeshFromBundle(const proto::ElementBundle bundle);

// A matcher for ElementBundleAdds.
MATCHER_P(BundleAddsMatchesExactly, expected, "") {
  if (arg.element_bundle_add_size() != expected.element_bundle_add_size()) {
    *result_listener << "bad count, expected: "
                     << expected.element_bundle_add_size()
                     << ", actual: " << arg.element_bundle_add_size();
    return false;
  }
  for (int i = 0; i < arg.element_bundle_add_size(); i++) {
    const auto& expected_bundle =
        expected.element_bundle_add(i).element_bundle();
    const auto& actual_bundle = arg.element_bundle_add(i).element_bundle();
    const auto& expected_below_uuid =
        expected.element_bundle_add(i).below_uuid();
    const auto& actual_below_uuid = arg.element_bundle_add(i).below_uuid();
    if (actual_bundle.uuid() != expected_bundle.uuid()) {
      *result_listener << "bad uuid, expected: " << expected_bundle.uuid()
                       << ", actual: " << actual_bundle.uuid();
      return false;
    }
    if (actual_below_uuid != expected_below_uuid) {
      *result_listener << "bad below_uuid, expected: " << expected_below_uuid
                       << ", actual: " << actual_below_uuid;
      return false;
    }
    EXPECT_EQ(actual_bundle.transform().SerializeAsString(),
              expected_bundle.transform().SerializeAsString());
    EXPECT_EQ(actual_bundle.element().SerializeAsString(),
              expected_bundle.element().SerializeAsString());
  }
  return true;
}

// A matcher for ElementBundle.
MATCHER_P(BundleMatchesExactly, expected, "") {
  if (arg.uuid() != expected.uuid()) {
    *result_listener << "bad uuid, expected: " << expected.uuid()
                     << ", actual: " << arg.uuid();
    return false;
  }
  EXPECT_EQ(arg.transform().SerializeAsString(),
            expected.transform().SerializeAsString());
  EXPECT_EQ(arg.element().SerializeAsString(),
            expected.element().SerializeAsString());
  return true;
}

// Matches an element bundle to a mesh, a stroke data, and a uuid.
MATCHER_P2(ElementBundleMatchesRect, expectedRect, expectedUUID, "") {
  if (arg.uuid() != expectedUUID) {
    *result_listener << "bad uuid, expected: " << expectedUUID
                     << ", actual: " << arg.uuid();
    return false;
  }
  Mesh flatMesh = getFlatMeshFromBundle(arg);
  if (!funcs::IsValidRectangleTriangulation(flatMesh, expectedRect)) {
    *result_listener << "mesh did not match rectangle: " << Str(expectedRect)
                     << ", vertices:\n";
    for (auto v : flatMesh.verts) {
      *result_listener << Str(v.position) << "\n";
    }
    return false;
  }
  return true;
}

MATCHER_P(ElementBundleMatchesAttributes, expectedAttributes, "") {
  if (arg.element().attributes().magic_erasable() !=
      expectedAttributes.magic_erasable) {
    *result_listener << "bad magic erasable attribute, expected: "
                     << expectedAttributes.magic_erasable << ", actual: "
                     << arg.element().attributes().magic_erasable();
    return false;
  }
  if (arg.element().attributes().selectable() !=
      expectedAttributes.selectable) {
    *result_listener << "bad selectable attribute, expected: "
                     << expectedAttributes.selectable
                     << ", actual: " << arg.element().attributes().selectable();
    return false;
  }
  return true;
}

//////////////////////// Source detail matchers ///////////////////////

// A Matcher for SourceDetails.  Checks that is from host and details (the
// uint32_t id) is correct.
//
// "hostSourceDetails" should be a uint32_t
MATCHER_P(SourceDetailsAreHost, hostSourceDetails, "") {
  return arg.origin() == proto::SourceDetails::HOST &&
         arg.host_source_details() == hostSourceDetails;
}

// A Matcher for Source details, checks that the source is the engine.
//
MATCHER(SourceDetailsAreEngine, "") {
  return arg.origin() == proto::SourceDetails::ENGINE;
}

}  // namespace ink
#endif  // INK_ENGINE_PUBLIC_TYPES_PUBLIC_TYPE_MATCHERS_H_
