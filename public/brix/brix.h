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

// Utility functions to mitigate the boilerplate involved in moving between
// SEngine data structures and their brix-compatible counterparts.

#ifndef INK_PUBLIC_BRIX_BRIX_H_
#define INK_PUBLIC_BRIX_BRIX_H_

#include "ink/engine/public/types/uuid.h"
#include "ink/engine/util/security.h"
#include "ink/proto/brix_portable_proto.pb.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {
namespace brix {

S_WARN_UNUSED_RESULT bool ElementBundleToBrixElementBundle(
    const ink::proto::ElementBundle& in, ink::proto::BrixElementBundle* out);

S_WARN_UNUSED_RESULT bool ElementMutationsToBrixElementMutation(
    const ink::proto::ElementTransformMutations& mutations,
    ink::proto::BrixElementMutation* out);

S_WARN_UNUSED_RESULT bool ElementTransformsToBrixElementMutation(
    const std::vector<UUID>& uuids,
    const std::vector<proto::AffineTransform>& transforms,
    ink::proto::BrixElementMutation* out);

S_WARN_UNUSED_RESULT bool BrixElementToElementBundle(
    const std::string& uuid, const std::string& encoded_element,
    const std::string& encoded_transform, ink::proto::ElementBundle* out);

S_WARN_UNUSED_RESULT bool AppendBrixElementMutation(
    const std::string& uuid, const std::string& encoded_transform,
    ink::proto::ElementTransformMutations* target);

}  // namespace brix
}  // namespace ink
#endif  // INK_PUBLIC_BRIX_BRIX_H_
