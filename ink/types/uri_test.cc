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

#include "ink/types/uri.h"

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "fuzztest/fuzztest.h"
#include "absl/hash/hash_testing.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "ink/types/fuzz_domains.h"

namespace ink {
namespace {

using ::testing::HasSubstr;

constexpr absl::StatusCode kInvalidArgument =
    absl::StatusCode::kInvalidArgument;

TEST(UriTest, SupportsAbslHash) {
  std::vector<std::string> uri_strings = {
      "",
      "ink://ink/brush-family:foo:1",
      "ink://baz/brush-family:foo:1",
  };

  std::vector<Uri> uris;
  for (const std::string& uri_string : uri_strings) {
    absl::StatusOr<Uri> uri = Uri::Parse(uri_string);
    ASSERT_EQ(uri.status(), absl::OkStatus());
    uris.push_back(*uri);
  }

  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly(uris));
}

TEST(UriTest, StringifyAssetType) {
  EXPECT_EQ(absl::StrCat(Uri::AssetType::kUninitialized), "uninitialized");
  EXPECT_EQ(absl::StrCat(Uri::AssetType::kBrushFamily), "brush-family");
  EXPECT_EQ(absl::StrCat(static_cast<Uri::AssetType>(42)), "AssetType(42)");
}

TEST(UriTest, DefaultCtor) {
  Uri uri;

  EXPECT_EQ(uri.GetRegName(), "");
  EXPECT_EQ(uri.GetAssetType(), Uri::AssetType::kUninitialized);
  EXPECT_EQ(uri.GetAssetName(), "");
  EXPECT_EQ(uri.GetRevisionNumber(), 0);
}

TEST(UriTest, ToNormalizedString) {
  absl::StatusOr<Uri> uri = Uri();
  EXPECT_EQ(uri->ToNormalizedString(), "");

  // Scheme is omitted.
  uri = Uri::Parse("ink://host/brush-family:marker:2");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->ToNormalizedString(), "//host/brush-family:marker:2");

  // Reg-name is omitted if it's "ink".
  uri = Uri::Parse("//ink/brush-family:marker");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->ToNormalizedString(), "/brush-family:marker");

  // Revision is omitted if it's 1.
  uri = Uri::Parse("/brush-family:marker:1");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->ToNormalizedString(), "/brush-family:marker");
}

TEST(UriTest, UriParse) {
  std::string test_uri = "/brush-family:start-test-family";
  absl::StatusOr<Uri> uri = Uri::Parse(test_uri);
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->ToNormalizedString(), test_uri);

  // Full URI scheme
  test_uri = "ink://ink/brush-family:highlighter:1";
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(test_uri).status());

  // URI scheme, no reg-name
  test_uri = "ink:/brush-family:highlighter:1";
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(test_uri).status());

  // No URI scheme, reg-name
  test_uri = "//reg/brush-family:highlighter:1";
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(test_uri).status());

  // No URI scheme, no reg-name
  test_uri = "/brush-family:highlighter:1";
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(test_uri).status());

  // No URI scheme, no reg-name, no revision
  test_uri = "/brush-family:highlighter";
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(test_uri).status());

  // scheme, no reg-name,
  test_uri = "INK:/brush-family:a";
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(test_uri).status());

  // empty string,
  test_uri = "";
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(test_uri).status());
}

TEST(UriTest, GetUriParts) {
  std::string test_uri = "ink://ink/brush-family:highlighter:13";
  auto uri = Uri::Parse(test_uri);
  ASSERT_EQ(absl::OkStatus(), uri.status());

  EXPECT_EQ(uri->GetRegName(), "ink");
  EXPECT_EQ(uri->GetAssetType(), Uri::AssetType::kBrushFamily);
  EXPECT_EQ(uri->GetAssetName(), "highlighter");
  EXPECT_EQ(uri->GetRevisionNumber(), 13);

  test_uri = "ink:/brush-family:highlighter:1";
  uri = Uri::Parse(test_uri);
  ASSERT_EQ(absl::OkStatus(), uri.status());

  EXPECT_EQ(uri->GetRegName(), "ink");
  EXPECT_EQ(uri->GetAssetType(), Uri::AssetType::kBrushFamily);
  EXPECT_EQ(uri->GetAssetName(), "highlighter");
  EXPECT_EQ(uri->GetRevisionNumber(), 1);

  test_uri = "ink://reg/brush-family:highlighter";
  uri = Uri::Parse(test_uri);
  ASSERT_EQ(absl::OkStatus(), uri.status());

  EXPECT_EQ(uri->GetRegName(), "reg");
  EXPECT_EQ(uri->GetAssetType(), Uri::AssetType::kBrushFamily);
  EXPECT_EQ(uri->GetAssetName(), "highlighter");
  EXPECT_EQ(uri->GetRevisionNumber(), 1);

  test_uri = "";
  uri = Uri::Parse(test_uri);
  ASSERT_EQ(absl::OkStatus(), uri.status());

  EXPECT_EQ(uri->GetRegName(), "");
  EXPECT_EQ(uri->GetAssetType(), Uri::AssetType::kUninitialized);
  EXPECT_EQ(uri->GetAssetName(), "");
  EXPECT_EQ(uri->GetRevisionNumber(), 0);
}

TEST(UriTest, ParseInvalidSchemeName) {
  std::string test_uri = "test://ink/brush-family:highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("start"));
}

TEST(UriTest, ParseRegNameMissingDoubleSlash) {
  std::string test_uri = "reg/brush-family:highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("start"));
}

TEST(UriTest, ParseInvalidRegName) {
  std::string test_uri = "ink://user@reg/brush-family:highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Reg-name"));
}

TEST(UriTest, ParseCapitalLetterNotPermittedInAssetType) {
  std::string test_uri = "ink://reg/brush-Family:highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Asset-type"));
}

TEST(UriTest, ParseInvalidAssetType) {
  std::string test_uri = "ink://reg/foobar:highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Invalid asset-type"));
}

TEST(UriTest, ParseInvalidAssetName) {
  std::string test_uri = "ink://reg/brush-family:highLighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Asset-name"));
}

TEST(UriTest, ParseAssetNameNoRevision) {
  std::string test_uri = "ink://reg/brush-family:highLighter";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Asset-name"));
}

TEST(UriTest, ParseZeroRevisionIsInvalid) {
  std::string test_uri = "ink://ink/brush-family:highlighter:0";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Revision"));
}

TEST(UriTest, ParseLeadingZeroRevisionIsInvalid) {
  std::string test_uri = "ink://ink/brush-family:highlighter:01";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Revision"));
}

TEST(UriTest, ParseRevisionTooLong) {
  std::string test_uri = "ink://ink/brush-family:highlighter:1234567890";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Revision"));
}

TEST(UriTest, ParseNegativeRevisionIsInvalid) {
  std::string test_uri = "ink://ink/brush-family:highlighter:-1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Revision"));
}

TEST(UriTest, ParseRevisionMustBeNumber) {
  std::string test_uri = "ink://ink/brush-family:highlighter:a";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Revision"));
}

TEST(UriTest, ParseAssetNameIsNumber) {
  std::string test_uri = "ink://ink/highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Asset-name"));
}

TEST(UriTest, ParseAssetNameIsEmpty) {
  std::string test_uri = "ink://ink/brush-family:";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("asset-name"));
}

TEST(UriTest, ParseMissingAssetNameWithRevision) {
  std::string test_uri = "ink://ink/brush-family::1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("asset-name"));
}

TEST(UriTest, ParseEmptyRevision) {
  std::string test_uri = "ink:/brush-family:uxp:";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Revision"));
}

TEST(UriTest, ParseMissingAssetType) {
  std::string test_uri = "ink://reg";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("asset-type"));
}

TEST(UriTest, ParseRegNameWithoutLeadingDoubleSlash) {
  std::string test_uri = "ink/brush-family:highlighter";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("must start with"));
}

TEST(UriTest, ParseMissingRegName) {
  std::string test_uri = "///brush-family:highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("reg-name"));
}

TEST(UriTest, ParseEmptyAssetType) {
  std::string test_uri = "//ink/:highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Non-empty asset-type"));
}

TEST(UriTest, ParseEmptyAssetName) {
  std::string test_uri = "//ink/brush-family";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Non-empty asset-name"));
}

TEST(UriTest, ParseNoSlashBeforeAssetName) {
  std::string test_uri = "//ink/brush-family/highlighter:1";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Asset-name must start"));
}

TEST(UriTest, ParseFragmentProvided) {
  // URI fragments not currently supported

  std::string test_uri = "//ink/brush-family:highlighter:1#frag";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Expected end of string"));
}

TEST(UriTest, ParseOnlyScheme) {
  std::string test_uri = "ink:";
  absl::Status status = Uri::Parse(test_uri).status();
  EXPECT_EQ(status.code(), kInvalidArgument);
  EXPECT_THAT(status.message(), HasSubstr("Asset-type and asset-name"));
}

TEST(UriTest, GetRevisionNumber) {
  // The default revision number is 1.
  absl::StatusOr<Uri> uri = Uri::Parse("/brush-family:marker");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->GetRevisionNumber(), 1);

  // If an explicit revision number is given, we should parse it.
  uri = Uri::Parse("/brush-family:marker:1");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->GetRevisionNumber(), 1);

  uri = Uri::Parse("/brush-family:marker:42");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->GetRevisionNumber(), 42);

  uri = Uri::Parse("/brush-family:marker:987654321");
  ASSERT_EQ(uri.status(), absl::OkStatus());
  EXPECT_EQ(uri->GetRevisionNumber(), 987654321);
}

void CanParseAnyValidUriString(absl::string_view uri) {
  EXPECT_EQ(absl::OkStatus(), Uri::Parse(uri).status());
}
FUZZ_TEST(UriTest, CanParseAnyValidUriString).WithDomains(ValidUriString());

void CanTryToParseAnyStringWithoutCrashing(absl::string_view str) {
  Uri::Parse(str).IgnoreError();
}
FUZZ_TEST(UriTest, CanTryToParseAnyStringWithoutCrashing)
    .WithDomains(fuzztest::String());  // includes non-UTF8 strings

void UriParseNormalizedStringRoundTrip(const Uri& uri) {
  absl::StatusOr<Uri> round_tripped = Uri::Parse(uri.ToNormalizedString());
  ASSERT_EQ(round_tripped.status(), absl::OkStatus());
  EXPECT_THAT(*round_tripped, uri);
}
FUZZ_TEST(UriTest, UriParseNormalizedStringRoundTrip)
    .WithDomains(ArbitraryUri());

}  // namespace
}  // namespace ink
