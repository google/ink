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

#include "ink/rendering/skia/native/internal/create_mesh_specification.h"

#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "ink/geometry/mesh_format.h"
#include "ink/rendering/skia/common_internal/mesh_specification_data.h"
#include "ink/strokes/internal/stroke_vertex.h"
#include "include/core/SkMesh.h"
#include "include/core/SkRefCnt.h"

namespace ink::skia_native_internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::ink::skia_common_internal::MeshSpecificationData;
using ::ink::strokes_internal::StrokeVertex;
using ::testing::HasSubstr;
using ::testing::NotNull;
using ::testing::Pointer;

TEST(CreateMeshSpecificationTest, MakeForInProgressStroke) {
  EXPECT_THAT(CreateMeshSpecification(
                  MeshSpecificationData::CreateForInProgressStroke()),
              IsOkAndHolds(Pointer(NotNull())));
}

TEST(CreateMeshSpecificationTest, MakeForStrokeWithFullMeshFormat) {
  absl::StatusOr<MeshSpecificationData> data =
      MeshSpecificationData::CreateForStroke(StrokeVertex::FullMeshFormat());
  ASSERT_THAT(data, IsOk());
  EXPECT_THAT(CreateMeshSpecification(*data), IsOkAndHolds(Pointer(NotNull())));
}

// Returns a format identical to `starting_format` except that an attribute with
// `attribute_id_to_skip` will be removed.
MeshFormat MakeFormatWithSkippedAttribute(
    const MeshFormat& starting_format,
    MeshFormat::AttributeId attribute_id_to_skip) {
  std::vector<std::pair<MeshFormat::AttributeType, MeshFormat::AttributeId>>
      new_types_and_ids;
  for (const MeshFormat::Attribute& attribute : starting_format.Attributes()) {
    if (attribute.id == attribute_id_to_skip) continue;
    new_types_and_ids.push_back({attribute.type, attribute.id});
  }

  auto new_format =
      MeshFormat::Create(new_types_and_ids, starting_format.GetIndexFormat());
  ABSL_CHECK_OK(new_format);
  return *new_format;
}

TEST(CreateMeshSpecificationTest, MakeForStrokeWithoutHslColorShift) {
  absl::StatusOr<MeshSpecificationData> data =
      MeshSpecificationData::CreateForStroke(MakeFormatWithSkippedAttribute(
          StrokeVertex::FullMeshFormat(),
          MeshFormat::AttributeId::kColorShiftHsl));
  ASSERT_THAT(data, IsOk());
  EXPECT_THAT(CreateMeshSpecification(*data), IsOkAndHolds(Pointer(NotNull())));
}

TEST(CreateMeshSpecificationTest, TryCreateWithShaderSourceError) {
  EXPECT_THAT(CreateMeshSpecification({
                  .attributes = {{
                      .type = MeshSpecificationData::AttributeType::kFloat2,
                      .name = "position",
                  }},
                  .vertex_stride = 8,
                  .varyings = {},
                  .uniforms = {},
                  .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      // ERROR - Missing return statement.
                    }
                  )",
                  .fragment_shader_source = R"(
                    float2 main(const Varyings varyings) {
                      return varyings.position;
                    }
                  )",
              }),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("`SkMeshSpecification::Make()` failed")));
}

TEST(CreateMeshSpecificationTest, TryCreateWithUnexpectedUniform) {
  EXPECT_THAT(
      CreateMeshSpecification({
          .attributes = {{
              .type = MeshSpecificationData::AttributeType::kFloat2,
              .name = "position",
          }},
          .vertex_stride = 8,
          .varyings = {},
          .uniforms = {},
          .vertex_shader_source = R"(
                    uniform float2 uPositionOffset;

                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = uPositionOffset + attributes.position;
                      return varyings;
                    }
                  )",
          .fragment_shader_source = R"(
                    float2 main(const Varyings varyings) {
                      return varyings.position;
                    }
                  )",
      }),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("uniform count")));
}

TEST(CreateMeshSpecificationTest, TryCreateWithMissingExpectedUniform) {
  EXPECT_THAT(CreateMeshSpecification({
                  .attributes = {{
                      .type = MeshSpecificationData::AttributeType::kFloat2,
                      .name = "position",
                  }},
                  .vertex_stride = 8,
                  .varyings = {},
                  .uniforms = {{
                      .type = MeshSpecificationData::UniformType::kFloat4,
                      .id = MeshSpecificationData::UniformId::kBrushColor,
                  }},
                  .vertex_shader_source = R"(
                    uniform float2 uPositionOffset;

                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = uPositionOffset + attributes.position;
                      return varyings;
                    }
                  )",
                  .fragment_shader_source = R"(
                    float2 main(const Varyings varyings) {
                      return varyings.position;
                    }
                  )",
              }),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("does not have uniform")));
}

TEST(CreateMeshSpecificationTest, TryCreateWithUnexpectedUniformType) {
  EXPECT_THAT(CreateMeshSpecification({
                  .attributes = {{
                      .type = MeshSpecificationData::AttributeType::kFloat2,
                      .name = "position",
                  }},
                  .vertex_stride = 8,
                  .varyings = {},
                  .uniforms = {{
                      .type = MeshSpecificationData::UniformType::kFloat4,
                      .id = MeshSpecificationData::UniformId::kBrushColor,
                  }},
                  .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      return varyings;
                    }
                  )",
                  .fragment_shader_source = R"(
                    layout(color) uniform float3 uBrushColor;  // WRONG TYPE

                    float2 main(const Varyings varyings, out float4 color) {
                      color = float4(uBrushColor, 1.0);
                      return varyings.position;
                    }
                  )",
              }),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unexpected type for uniform")));
}

TEST(CreateMeshSpecificationDeathTest, MakeWithNonEnumeratorAttributeType) {
  EXPECT_DEATH_IF_SUPPORTED(
      CreateMeshSpecification(
          {
              .attributes = {{
                  .type = static_cast<MeshSpecificationData::AttributeType>(99),
                  .name = "position",
              }},
              .vertex_stride = 8,
              .varyings = {},
              .uniforms = {},
              .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      return varyings;
                    }
                  )",
              .fragment_shader_source = R"(
                    float2 main(const Varyings varyings) {
                      return varyings.position;
                    }
                  )",
          })
          .IgnoreError(),
      "Non-enumerator value");
}

TEST(CreateMeshSpecificationDeathTest, MakeWithEmptyAttributeName) {
  EXPECT_DEATH_IF_SUPPORTED(
      CreateMeshSpecification(
          {
              .attributes = {{
                  .type = MeshSpecificationData::AttributeType::kFloat2,
                  .name = "",
              }},
              .vertex_stride = 8,
              .varyings = {},
              .uniforms = {},
              .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      return varyings;
                    }
                  )",
              .fragment_shader_source = R"(
                    float2 main(const Varyings varyings) {
                      return varyings.position;
                    }
                  )",
          })
          .IgnoreError(),
      "");
}

TEST(CreateMeshSpecificationDeathTest, MakeWithNonEnumeratorVaryingType) {
  EXPECT_DEATH_IF_SUPPORTED(
      CreateMeshSpecification(
          {
              .attributes = {{
                  .type = MeshSpecificationData::AttributeType::kFloat2,
                  .name = "position",
              }},
              .vertex_stride = 8,
              .varyings = {{
                  .type = static_cast<MeshSpecificationData::VaryingType>(99),
                  .name = "position",
              }},
              .uniforms = {},
              .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      return varyings;
                    }
                  )",
              .fragment_shader_source = R"(
                    float2 main(const Varyings varyings) {
                      return varyings.position;
                    }
                  )",
          })
          .IgnoreError(),
      "Non-enumerator value");
}

TEST(CreateMeshSpecificationDeathTest, MakeWithEmptyVaryingName) {
  EXPECT_DEATH_IF_SUPPORTED(
      CreateMeshSpecification(
          {
              .attributes = {{
                  .type = MeshSpecificationData::AttributeType::kFloat2,
                  .name = "position",
              }},
              .vertex_stride = 8,
              .varyings = {{
                  .type = MeshSpecificationData::VaryingType::kFloat2,
                  .name = "",
              }},
              .uniforms = {},
              .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      return varyings;
                    }
                  )",
              .fragment_shader_source = R"(
                    float2 main(const Varyings varyings, out float4 color) {
                      return varyings.position;
                    }
                  )",
          })
          .IgnoreError(),
      "");
}

TEST(CreateMeshSpecificationDeathTest, MakeWithNonEnumeratorUniformType) {
  EXPECT_DEATH_IF_SUPPORTED(
      CreateMeshSpecification(
          {
              .attributes = {{
                  .type = MeshSpecificationData::AttributeType::kFloat2,
                  .name = "position",
              }},
              .vertex_stride = 8,
              .varyings = {},
              .uniforms = {{
                  .type = static_cast<MeshSpecificationData::UniformType>(99),
                  .id = MeshSpecificationData::UniformId::kBrushColor,
              }},
              .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      return varyings;
                    }
                  )",
              .fragment_shader_source = R"(
                    layout(color) uniform float4 uBrushColor;
                    float2 main(const Varyings varyings, out float4 color) {
                      color = float4(uBrushColor.rgb * uBrushColor.a,
                                     uBrushColor.a);
                      return varyings.position;
                    }
                  )",
          })
          .IgnoreError(),
      "Non-enumerator value");
}

TEST(CreateMeshSpecificationDeathTest, MakeWithNonEnumeratorUniformId) {
  EXPECT_DEATH_IF_SUPPORTED(
      CreateMeshSpecification(
          {
              .attributes = {{
                  .type = MeshSpecificationData::AttributeType::kFloat2,
                  .name = "position",
              }},
              .vertex_stride = 8,
              .varyings = {},
              .uniforms = {{
                  .type = MeshSpecificationData::UniformType::kFloat4,
                  .id = static_cast<MeshSpecificationData::UniformId>(99),
              }},
              .vertex_shader_source = R"(
                    Varyings main(const Attributes attributes) {
                      Varyings varyings;
                      varyings.position = attributes.position;
                      return varyings;
                    }
                  )",
              .fragment_shader_source = R"(
                    layout(color) uniform float4 uBrushColor;
                    float2 main(const Varyings varyings, out float4 color) {
                      color = float4(uBrushColor.rgb * uBrushColor.a,
                                     uBrushColor.a);
                      return varyings.position;
                    }
                  )",
          })
          .IgnoreError(),
      "Non-enumerator value");
}

}  // namespace
}  // namespace ink::skia_native_internal
