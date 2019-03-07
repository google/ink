/*
 * Copyright 2019 Google LLC
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

#include "net/proto2/public/repeated_field.h"
#include "security/fuzzing/blaze/proto_message_mutator.h"
#include "ink/engine/geometry/algorithms/boolean_operation.h"
#include "ink/engine/geometry/algorithms/boolean_operation_fuzzer.proto.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/primitives/polygon.h"
#include "ink/proto/geometry.proto.h"

namespace ink {
namespace geometry {
namespace {

bool IsPolygonSelfIntersecting(const Polygon& polygon) {
  for (int i = 0; i < polygon.Size(); ++i) {
    for (int j = i + 2; j < polygon.Size(); ++j) {
      if (i == 0 && j == polygon.Size() - 1) continue;
      if (geometry::Intersects(polygon.GetSegment(i), polygon.GetSegment(j)))
        return true;
    }
  }
  return false;
}

bool ParsePolygon(const proto2::RepeatedPtrField<proto::Point>& proto_points,
                  Polygon* polygon) {
  if (proto_points.size() < 3) return false;

  std::vector<glm::vec2> points;
  points.reserve(proto_points.size());
  for (const auto& p : proto_points) {
    if (!p.has_x() || !p.has_y()) return false;

    points.emplace_back(p.x(), p.y());
  }

  *polygon = Polygon(std::move(points));
  return polygon->SignedArea() >= 0 && !IsPolygonSelfIntersecting(*polygon);
}

void PerformBooleanOperation(const proto::BooleanOperation& input) {
  if (!input.has_operation()) return;

  Polygon lhs;
  Polygon rhs;
  if (!ParsePolygon(input.lhs_polygon(), &lhs) ||
      !ParsePolygon(input.rhs_polygon(), &rhs))
    return;

  switch (input.operation()) {
    case proto::BooleanOperation_Operation::
        BooleanOperation_Operation_DIFFERENCE:
      Difference(lhs, rhs);
      break;
    case proto::BooleanOperation_Operation::
        BooleanOperation_Operation_INTERSECTION:
      Intersection(lhs, rhs);
      break;
    case proto::BooleanOperation_Operation::BooleanOperation_Operation_UNKNOWN:
      break;
  }
}

}  // namespace
}  // namespace geometry
}  // namespace ink

DEFINE_PROTO_FUZZER(const ink::proto::BooleanOperation& input) {
  ink::geometry::PerformBooleanOperation(input);
}
