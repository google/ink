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

#include "ink/engine/geometry/primitives/primitive_test_helpers.h"

#include "third_party/absl/types/optional.h"
#include "ink/engine/util/dbg/str.h"
#include "ink/engine/util/iterator/cyclic_iterator.h"

namespace ink {
namespace {

using geometry::Polygon;
using geometry::Triangle;

using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatcherInterface;
using ::testing::Matches;
using ::testing::MatchResultListener;

bool IsEqual(glm::vec2 expected, glm::vec2 actual) {
  return Matches(FloatEq(expected.x))(actual.x) &&
         Matches(FloatEq(expected.y))(actual.y);
}
bool IsEqual(glm::vec3 expected, glm::vec3 actual) {
  return Matches(FloatEq(expected.x))(actual.x) &&
         Matches(FloatEq(expected.y))(actual.y) &&
         Matches(FloatEq(expected.z))(actual.z);
}
bool IsEqual(glm::vec4 expected, glm::vec4 actual) {
  return Matches(FloatEq(expected.x))(actual.x) &&
         Matches(FloatEq(expected.y))(actual.y) &&
         Matches(FloatEq(expected.z))(actual.z) &&
         Matches(FloatEq(expected.w))(actual.w);
}
bool IsEqual(const glm::mat4& expected, const glm::mat4& actual) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      if (!Matches(FloatEq(expected[i][j]))(actual[i][j])) return false;
  return true;
}
bool IsEqual(const Segment& expected, const Segment& actual) {
  return IsEqual(expected.from, actual.from) && IsEqual(expected.to, actual.to);
}
bool IsEqual(const Triangle& expected, const Triangle& actual) {
  return IsEqual(expected[0], actual[0]) && IsEqual(expected[1], actual[1]) &&
         IsEqual(expected[2], actual[2]);
}
bool IsEqual(const Rect& expected, const Rect& actual) {
  return IsEqual(expected.from, actual.from) && IsEqual(expected.to, actual.to);
}
bool IsEqual(const RotRect& expected, const RotRect& actual) {
  return IsEqual(expected.Center(), actual.Center()) &&
         IsEqual(expected.Dim(), actual.Dim()) &&
         Matches(FloatEq(expected.Rotation()))(actual.Rotation());
}
bool IsEqual(const std::vector<glm::vec2>& expected,
             const std::vector<glm::vec2>& actual) {
  if (expected.size() != actual.size()) return false;
  for (int i = 0; i < expected.size(); ++i)
    if (!IsEqual(expected[i], actual[i])) return false;
  return true;
}
template <typename T>
class TypeEqMatcher : public MatcherInterface<T> {
 public:
  explicit TypeEqMatcher(T expected) : expected_(std::move(expected)) {}

  bool MatchAndExplain(T actual,
                       MatchResultListener* result_listener) const override {
    *result_listener << Str(actual);
    return IsEqual(expected_, actual);
  }

  void DescribeTo(::std::ostream* os) const override {
    *os << Substitute("is approximately $0", Str(expected_));
  }
  void DescribeNegationTo(::std::ostream* os) const override {
    *os << Substitute("isn't approximately $0", Str(expected_));
  }

 private:
  T expected_;
};

bool IsNear(glm::vec2 expected, glm::vec2 actual, float max_abs_error) {
  return Matches(FloatNear(expected.x, max_abs_error))(actual.x) &&
         Matches(FloatNear(expected.y, max_abs_error))(actual.y);
}
bool IsNear(glm::vec3 expected, glm::vec3 actual, float max_abs_error) {
  return Matches(FloatNear(expected.x, max_abs_error))(actual.x) &&
         Matches(FloatNear(expected.y, max_abs_error))(actual.y) &&
         Matches(FloatNear(expected.z, max_abs_error))(actual.z);
}
bool IsNear(glm::vec4 expected, glm::vec4 actual, float max_abs_error) {
  return Matches(FloatNear(expected.x, max_abs_error))(actual.x) &&
         Matches(FloatNear(expected.y, max_abs_error))(actual.y) &&
         Matches(FloatNear(expected.z, max_abs_error))(actual.z) &&
         Matches(FloatNear(expected.w, max_abs_error))(actual.w);
}
bool IsNear(const glm::mat4& expected, const glm::mat4& actual,
            float max_abs_error) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      if (!Matches(FloatNear(expected[i][j], max_abs_error))(actual[i][j]))
        return false;
  return true;
}
bool IsNear(const Segment& expected, const Segment& actual,
            float max_abs_error) {
  return IsNear(expected.from, actual.from, max_abs_error) &&
         IsNear(expected.to, actual.to, max_abs_error);
}
bool IsNear(const Triangle& expected, const Triangle& actual,
            float max_abs_error) {
  return IsNear(expected[0], actual[0], max_abs_error) &&
         IsNear(expected[1], actual[1], max_abs_error) &&
         IsNear(expected[2], actual[2], max_abs_error);
}
bool IsNear(const Rect& expected, const Rect& actual, float max_abs_error) {
  return IsNear(expected.from, actual.from, max_abs_error) &&
         IsNear(expected.to, actual.to, max_abs_error);
}
bool IsNear(const RotRect& expected, const RotRect& actual,
            float max_abs_error) {
  return IsNear(expected.Center(), actual.Center(), max_abs_error) &&
         IsNear(expected.Dim(), actual.Dim(), max_abs_error) &&
         Matches(FloatNear(expected.Rotation(), max_abs_error))(
             actual.Rotation());
}
bool IsNear(const std::vector<glm::vec2>& expected,
            const std::vector<glm::vec2>& actual, float max_abs_error) {
  if (expected.size() != actual.size()) return false;
  for (int i = 0; i < expected.size(); ++i)
    if (!IsNear(expected[i], actual[i], max_abs_error)) return false;
  return true;
}
template <typename T>
class TypeNearMatcher : public MatcherInterface<T> {
 public:
  explicit TypeNearMatcher(T expected, float max_abs_error)
      : expected_(std::move(expected)), max_abs_error_(max_abs_error) {}

  bool MatchAndExplain(T actual,
                       MatchResultListener* result_listener) const override {
    *result_listener << Str(actual);
    return IsNear(expected_, actual, max_abs_error_);
  }

  void DescribeTo(::std::ostream* os) const override {
    *os << Substitute("is approximately $0 (absolute error <= $1)",
                      Str(expected_), max_abs_error_);
  }
  void DescribeNegationTo(::std::ostream* os) const override {
    *os << Substitute("isn't approximately $0 (absolute error > $1)",
                      Str(expected_), max_abs_error_);
  }

 private:
  T expected_;
  float max_abs_error_;
};

class PolygonEqOrNearMatcher : public MatcherInterface<Polygon> {
 public:
  explicit PolygonEqOrNearMatcher(Polygon expected)
      : expected_(std::move(expected)) {}
  PolygonEqOrNearMatcher(Polygon expected, float max_abs_error)
      : expected_(std::move(expected)), max_abs_error_(max_abs_error) {}

  bool MatchAndExplain(Polygon actual,
                       MatchResultListener* result_listener) const override {
    *result_listener << Str(actual);

    auto start_it =
        MakeCyclicIterator(actual.Points().begin(), actual.Points().end());
    do {
      bool all_match = true;
      auto inner_it = start_it;
      auto expected_it = expected_.Points().begin();
      do {
        if (max_abs_error_
                ? !IsNear(*expected_it, *inner_it, max_abs_error_.value())
                : !IsEqual(*expected_it, *inner_it)) {
          all_match = false;
          break;
        }
        ++expected_it;
        ++inner_it;
      } while (inner_it != start_it);

      if (all_match) return true;
      ++start_it;
    } while (start_it.BaseCurrent() != actual.Points().begin());

    return false;
  }

  void DescribeTo(::std::ostream* os) const override {
    *os << ConstructDescription(false);
  }
  void DescribeNegationTo(::std::ostream* os) const override {
    *os << ConstructDescription(true);
  }

 private:
  std::string ConstructDescription(bool negated) const {
    auto s = Substitute("$0 approximately equal to a circular shift of $1",
                        negated ? "isn't" : "is", Str(expected_));
    if (max_abs_error_)
      s += Substitute(" (absolute error $0 $1)",
                      negated ? ">" : "<=", max_abs_error_.value());
    return s;
  }

  Polygon expected_;
  absl::optional<float> max_abs_error_;
};
}  // namespace

Matcher<glm::vec2> Vec2Eq(glm::vec2 v) {
  return MakeMatcher(new TypeEqMatcher<glm::vec2>(v));
}
Matcher<glm::vec3> Vec3Eq(glm::vec3 v) {
  return MakeMatcher(new TypeEqMatcher<glm::vec3>(v));
}
Matcher<glm::vec4> Vec4Eq(glm::vec4 v) {
  return MakeMatcher(new TypeEqMatcher<glm::vec4>(v));
}
Matcher<glm::mat4> Mat4Eq(glm::mat4 m) {
  return MakeMatcher(new TypeEqMatcher<glm::mat4>(m));
}
Matcher<Segment> SegmentEq(Segment s) {
  return MakeMatcher(new TypeEqMatcher<Segment>(s));
}
Matcher<Triangle> TriangleEq(Triangle t) {
  return MakeMatcher(new TypeEqMatcher<Triangle>(t));
}
Matcher<Rect> RectEq(Rect r) { return MakeMatcher(new TypeEqMatcher<Rect>(r)); }
Matcher<RotRect> RotRectEq(RotRect r) {
  return MakeMatcher(new TypeEqMatcher<RotRect>(r));
}
Matcher<std::vector<glm::vec2>> PolylineEq(std::vector<glm::vec2> p) {
  return MakeMatcher(new TypeEqMatcher<std::vector<glm::vec2>>(p));
}

Matcher<glm::vec2> Vec2Near(glm::vec2 v, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<glm::vec2>(v, max_abs_error));
}
Matcher<glm::vec3> Vec3Near(glm::vec3 v, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<glm::vec3>(v, max_abs_error));
}
Matcher<glm::vec4> Vec4Near(glm::vec4 v, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<glm::vec4>(v, max_abs_error));
}
Matcher<glm::mat4> Mat4Near(glm::mat4 m, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<glm::mat4>(m, max_abs_error));
}
Matcher<Segment> SegmentNear(Segment s, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<Segment>(s, max_abs_error));
}
Matcher<Triangle> TriangleNear(Triangle t, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<Triangle>(t, max_abs_error));
}
Matcher<Rect> RectNear(Rect r, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<Rect>(r, max_abs_error));
}
Matcher<RotRect> RotRectNear(RotRect r, float max_abs_error) {
  return MakeMatcher(new TypeNearMatcher<RotRect>(r, max_abs_error));
}
Matcher<std::vector<glm::vec2>> PolylineNear(std::vector<glm::vec2> p,
                                             float max_abs_error) {
  return MakeMatcher(
      new TypeNearMatcher<std::vector<glm::vec2>>(p, max_abs_error));
}

Matcher<Polygon> PolygonEq(std::vector<glm::vec2> p) {
  return MakeMatcher(new PolygonEqOrNearMatcher(Polygon(p)));
}
Matcher<Polygon> PolygonEq(Polygon p) {
  return MakeMatcher(new PolygonEqOrNearMatcher(p));
}
Matcher<Polygon> PolygonNear(std::vector<glm::vec2> p, float max_abs_error) {
  return MakeMatcher(new PolygonEqOrNearMatcher(Polygon(p), max_abs_error));
}
Matcher<Polygon> PolygonNear(Polygon p, float max_abs_error) {
  return MakeMatcher(new PolygonEqOrNearMatcher(p, max_abs_error));
}
}  // namespace ink
