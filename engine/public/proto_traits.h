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

#ifndef INK_ENGINE_PUBLIC_PROTO_TRAITS_H_
#define INK_ENGINE_PUBLIC_PROTO_TRAITS_H_

#include "third_party/absl/meta/type_traits.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/uuid.h"
#include "ink/proto/elements_portable_proto.pb.h"

namespace ink {

/**
 * Because protobuf classes are generated, we cannot add types and
 * helpers directly to the protobufs themselves. Much of the code to process
 * mutations is fairly boilerplate, except for the types and field names.
 *
 * Using these traits, we can treat the Element*Mutations protos as if they were
 * "duck-typed", and avoid duplication of large amounts of code.
 *
 * ProtoTraitsBase is the base class for all of these traits, supplying types
 * that can be easily determined for all of the Mutations protos.
 */
template <typename T>
class ProtoTraitsBase {
 public:
  // Type of the repeated field containing individual mutations.
  using MutationType = typename T::Mutation;
};

template <typename T>
class ProtoTraits {
  // MutationType is expected to contain a pair of (UUID, Value).
  // The UUID field is named "uuid", but the type and name of the
  // value field changes from proto to proto.
  //
  // Each specialization of this type should provide these things:
  //
  //   ValueType:       the type of the Value field.
  //
  //   GetValue:        a getter for the Value field.
  //
  //   SetValue:        a setter for the Value field.
  //
  //   ApplyToDocument: a method to forward the mutations uuids and values to
  //                    the correct Document::SetElement*Impl method.
  //
  // NOTE: ValueType (and possibly the getter and setter) can be simplified
  //       as soon as C++17 support for 'auto return values' is supported.
  //
  // NOTE: ApplyToDocument cheats a little by making DocumentT a template
  //       parameter. This allows us to avoid including "document.h" which would
  //       be a circular include.
};

template <>
class ProtoTraits<proto::ElementTransformMutations>
    : public ProtoTraitsBase<proto::ElementTransformMutations> {
 public:
  using ValueType = proto::AffineTransform;
  static inline ValueType GetValue(const MutationType& mutation) {
    return mutation.transform();
  }
  static inline void SetValue(MutationType* mutation, const ValueType& value) {
    *mutation->mutable_transform() = value;
  }

  template <typename DocumentT>
  static inline Status ApplyToDocument(
      DocumentT* document, const std::vector<UUID>& uuids,
      const std::vector<ValueType>& values,
      const proto::SourceDetails& source_details) {
    return document->SetElementTransformsImpl(uuids, values, source_details);
  }
};

template <>
class ProtoTraits<proto::ElementVisibilityMutations>
    : public ProtoTraitsBase<proto::ElementVisibilityMutations> {
 public:
  using ValueType = bool;
  static inline ValueType GetValue(const MutationType& mutation) {
    return mutation.visibility();
  }
  static inline void SetValue(MutationType* mutation, ValueType value) {
    mutation->set_visibility(value);
  }

  template <typename DocumentT>
  static inline Status ApplyToDocument(
      DocumentT* document, const std::vector<UUID>& uuids,
      const std::vector<ValueType>& values,
      const proto::SourceDetails& source_details) {
    return document->SetElementVisibilityImpl(uuids, values, source_details);
  }
};

template <>
class ProtoTraits<proto::ElementOpacityMutations>
    : public ProtoTraitsBase<proto::ElementOpacityMutations> {
 public:
  using ValueType = int32;
  static inline ValueType GetValue(const MutationType& mutation) {
    return mutation.opacity();
  }
  static inline void SetValue(MutationType* mutation, ValueType value) {
    mutation->set_opacity(value);
  }

  template <typename DocumentT>
  static inline Status ApplyToDocument(
      DocumentT* document, const std::vector<UUID>& uuids,
      const std::vector<ValueType>& values,
      const proto::SourceDetails& source_details) {
    return document->SetElementOpacityImpl(uuids, values, source_details);
  }
};

template <>
class ProtoTraits<proto::ElementZOrderMutations>
    : public ProtoTraitsBase<proto::ElementZOrderMutations> {
 public:
  using ValueType = std::string;
  static inline ValueType GetValue(const MutationType& mutation) {
    return mutation.below_uuid();
  }
  static inline void SetValue(MutationType* mutation, const ValueType& value) {
    mutation->set_below_uuid(value);
  }

  template <typename DocumentT>
  static inline Status ApplyToDocument(
      DocumentT* document, const std::vector<UUID>& uuids,
      const std::vector<ValueType>& values,
      const proto::SourceDetails& source_details) {
    return document->ChangeZOrderImpl(uuids, values, source_details);
  }
};

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_PROTO_TRAITS_H_
