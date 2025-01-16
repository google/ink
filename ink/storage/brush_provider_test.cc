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

#include "ink/storage/brush_provider.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/types/fuzz_domains.h"
#include "ink/types/uri.h"

namespace ink {
namespace {

using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::HasSubstr;

TEST(BrushProviderTest, GetStockMarkerBrush) {
  absl::StatusOr<Uri> uri = Uri::Parse("ink:/brush-family:marker:1");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  absl::StatusOr<BrushFamily> family = BrushProvider().GetBrushFamily(*uri);
  ASSERT_EQ(family.status(), absl::OkStatus());
  EXPECT_THAT(
      family->GetCoats(),
      ElementsAre(Field(&BrushCoat::tips,
                        ElementsAre(Field(&BrushTip::corner_rounding, 1.0f)))));
  EXPECT_EQ(family->GetUri(), *uri);
}

TEST(BrushProviderTest, InvalidBrushFamilyUri) {
  absl::Status not_brush_family =
      BrushProvider().GetBrushFamily(Uri()).status();
  EXPECT_EQ(not_brush_family.code(), absl::StatusCode::kInvalidArgument);
  EXPECT_THAT(not_brush_family.message(), HasSubstr("not a brush family URI"));
}

TEST(BrushProviderTest, UnknownStockBrushAssetName) {
  {
    absl::StatusOr<Uri> uri = Uri::Parse("/brush-family:foobar");
    ASSERT_EQ(uri.status(), absl::OkStatus());
    absl::Status unknown_name = BrushProvider().GetBrushFamily(*uri).status();
    EXPECT_EQ(unknown_name.code(), absl::StatusCode::kNotFound);
    EXPECT_THAT(unknown_name.message(),
                HasSubstr("unknown stock brush asset name: foobar"));
  }

  {
    absl::StatusOr<Uri> uri = Uri::Parse("ink://ink/brush-family:xyzzy");
    ASSERT_EQ(uri.status(), absl::OkStatus());
    absl::Status unknown_name = BrushProvider().GetBrushFamily(*uri).status();
    EXPECT_EQ(unknown_name.code(), absl::StatusCode::kNotFound);
    EXPECT_THAT(unknown_name.message(),
                HasSubstr("unknown stock brush asset name: xyzzy"));
  }
}

TEST(BrushProviderTest, UnknownStockBrushRevision) {
  absl::StatusOr<Uri> uri = Uri::Parse("/brush-family:marker:987654321");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  absl::Status unknown_revision = BrushProvider().GetBrushFamily(*uri).status();
  EXPECT_EQ(unknown_revision.code(), absl::StatusCode::kNotFound);
  EXPECT_THAT(unknown_revision.message(),
              HasSubstr("unknown stock 'marker' brush revision: 987654321"));
}

TEST(BrushProviderTest, GetNonInkBrushFamilyFromBaseBrushProvider) {
  absl::StatusOr<Uri> uri = Uri::Parse("//gshoe/brush-family:footsteps");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  absl::Status unknown_family = BrushProvider().GetBrushFamily(*uri).status();
  EXPECT_EQ(unknown_family.code(), absl::StatusCode::kNotFound);
  EXPECT_THAT(unknown_family.message(), HasSubstr("unknown brush family"));
}

void CanTryToGetBrushFamilyWithoutCrashing(const Uri& uri) {
  BrushProvider().GetBrushFamily(uri).IgnoreError();
}
FUZZ_TEST(BrushProviderTest, CanTryToGetBrushFamilyWithoutCrashing)
    .WithDomains(ArbitraryUri());

class TestBrushProvider : public BrushProvider {
 protected:
  absl::StatusOr<BrushFamily> GetClientBrushFamily(
      const Uri& uri) const override {
    EXPECT_EQ(uri.GetRegName(), "test");
    EXPECT_EQ(uri.GetAssetType(), Uri::AssetType::kBrushFamily);
    return BrushFamily::Create(BrushTip{.pinch = 0.75f}, BrushPaint{}, uri);
  };
};

TEST(BrushProviderTest, GetStockInkBrushFamilyFromBrushProviderSubclass) {
  absl::StatusOr<Uri> uri = Uri::Parse("//ink/brush-family:marker");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  absl::StatusOr<BrushFamily> family = TestBrushProvider().GetBrushFamily(*uri);
  ASSERT_EQ(family.status(), absl::OkStatus());
  EXPECT_THAT(family->GetCoats(),
              ElementsAre(Field(&BrushCoat::tips,
                                ElementsAre(Field(&BrushTip::pinch, 0.0f)))));
}

TEST(BrushProviderTest, GetNonInkBrushFamilyFromBrushProviderSubclass) {
  absl::StatusOr<Uri> uri = Uri::Parse("//test/brush-family:pinched");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  absl::StatusOr<BrushFamily> family = TestBrushProvider().GetBrushFamily(*uri);
  ASSERT_EQ(family.status(), absl::OkStatus());
  EXPECT_THAT(family->GetCoats(),
              ElementsAre(Field(&BrushCoat::tips,
                                ElementsAre(Field(&BrushTip::pinch, 0.75f)))));
}

}  // namespace
}  // namespace ink
