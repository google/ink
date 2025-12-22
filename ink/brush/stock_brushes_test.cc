#include "ink/brush/stock_brushes.h"

#include <string>
#include <utility>
#include <variant>

#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/stock_brushes_test_params.h"

namespace ink::stock_brushes {
namespace {

using ::testing::TestWithParam;
using ::testing::ValuesIn;

class StockBrushesTest : public TestWithParam<StockBrushesTestParam> {};

TEST_P(StockBrushesTest, PredictionFadeOutBehaviorOccursOncePerBrushTip) {
  const auto& [name, family] = GetParam();
  const absl::flat_hash_set<std::string> exceptions = {
      "heart_emoji_highlighter_1", "heart_emoji_highlighter_no_trail_1"};
  if (exceptions.contains(name)) {
    GTEST_SKIP() << "Skipping prediction check for " << name;
  }

  BrushBehavior prediction = PredictionFadeOutBehavior();
  for (const auto& coat : family.GetCoats()) {
    int count = 0;
    for (const auto& behavior : coat.tip.behaviors) {
      if (behavior == prediction) {
        count++;
      }
    }
    EXPECT_EQ(count, 1) << "for coat in family " << name;
  }
}

TEST_P(StockBrushesTest, TargetSizeMultiplierIsNotZero) {
  const auto& [name, family] = GetParam();
  const absl::flat_hash_set<std::string> exceptions = {
      "heart_emoji_highlighter_1", "heart_emoji_highlighter_no_trail_1"};
  if (exceptions.contains(name)) {
    GTEST_SKIP() << "Skipping size multiplier check for " << name;
  }

  for (const auto& coat : family.GetCoats()) {
    for (const auto& behavior : coat.tip.behaviors) {
      for (const auto& node : behavior.nodes) {
        if (std::holds_alternative<BrushBehavior::TargetNode>(node)) {
          const auto& target_node = std::get<BrushBehavior::TargetNode>(node);
          if (target_node.target == BrushBehavior::Target::kSizeMultiplier) {
            EXPECT_NE(target_node.target_modifier_range.front(), 0.f);
            EXPECT_NE(target_node.target_modifier_range.back(), 0.f);
          }
        }
      }
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    StockBrushes, StockBrushesTest, ValuesIn(GetParams()),
    [](const testing::TestParamInfo<StockBrushesTest::ParamType>& info) {
      return info.param.first;
    });

}  // namespace
}  // namespace ink::stock_brushes
