// Copyright 2024-2025 Google LLC
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

#ifndef INK_STORAGE_BRUSH_H_
#define INK_STORAGE_BRUSH_H_

#include "absl/status/statusor.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/storage/proto/brush.pb.h"

namespace ink {

// Populates the given proto by encoding the given brush object.
//
// The proto need not be empty before calling these; they will effectively clear
// the proto first.
void EncodeBrush(const Brush& brush, proto::Brush& brush_proto_out);
void EncodeBrushFamily(const BrushFamily& family,
                       proto::BrushFamily& family_proto_out);
void EncodeBrushCoat(const BrushCoat& coat, proto::BrushCoat& coat_proto_out);
void EncodeBrushPaint(const BrushPaint& paint,
                      proto::BrushPaint& paint_proto_out);
void EncodeBrushTip(const BrushTip& tip, proto::BrushTip& tip_proto_out);
void EncodeBrushBehaviorNode(const BrushBehavior::Node& node,
                             proto::BrushBehavior::Node& node_proto_out);

// Decodes the proto into a brush object. Returns an error if the proto is
// invalid.
absl::StatusOr<Brush> DecodeBrush(const proto::Brush& brush_proto);
absl::StatusOr<BrushFamily> DecodeBrushFamily(
    const proto::BrushFamily& family_proto);
// For the below decoding functions, note that only minimal validation is done
// on the proto.  Decoding is only *guaranteed* to succeed if the decoded struct
// would be valid to construct a BrushFamily with, but decoding *may* still
// succeed even in cases where trying to put the decoded struct into a
// BrushFamily would return an error.
absl::StatusOr<BrushCoat> DecodeBrushCoat(const proto::BrushCoat& coat_proto);
absl::StatusOr<BrushPaint> DecodeBrushPaint(
    const proto::BrushPaint& paint_proto);
absl::StatusOr<BrushTip> DecodeBrushTip(const proto::BrushTip& tip_proto);
absl::StatusOr<BrushBehavior::Node> DecodeBrushBehaviorNode(
    const proto::BrushBehavior::Node& node_proto);

}  // namespace ink

#endif  // INK_STORAGE_BRUSH_H_
