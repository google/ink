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

#ifndef INK_ENGINE_PUBLIC_PROTO_VALIDATORS_H_
#define INK_ENGINE_PUBLIC_PROTO_VALIDATORS_H_

#include "ink/engine/util/security.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

S_WARN_UNUSED_RESULT
bool ValidateProto(const proto::ElementBundle& unsafe_bundle);

S_WARN_UNUSED_RESULT
bool ValidateProtoForAdd(const proto::ElementBundle& unsafe_bundle);

S_WARN_UNUSED_RESULT
bool ValidateProto(const proto::ElementTransformMutations& unsafe_mutations);
S_WARN_UNUSED_RESULT
bool ValidateProto(const proto::ElementVisibilityMutations& unsafe_mutations);
S_WARN_UNUSED_RESULT
bool ValidateProto(const proto::ElementOpacityMutations& unsafe_mutations);
S_WARN_UNUSED_RESULT
bool ValidateProto(const proto::ElementZOrderMutations& unsafe_mutations);

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_PROTO_VALIDATORS_H_
