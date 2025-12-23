#include "ink/brush/stock_brushes_test_params.h"

#include <string>
#include <vector>

#include "ink/brush/brush_paint.h"
#include "ink/brush/stock_brushes.h"

namespace ink::stock_brushes {

std::vector<stock_brushes::StockBrushesTestParam> GetParams() {
  std::vector<StockBrushesTestParam> families = {
      {"marker_1", Marker(MarkerVersion::kV1)},
      {"pressure_pen_1", PressurePen(PressurePenVersion::kV1)},
      {"highlighter_1",
       Highlighter(BrushPaint::SelfOverlap::kAny, HighlighterVersion::kV1)},
      {"dashed_line_1", DashedLine(DashedLineVersion::kV1)},
      {"heart_emoji_highlighter_1",
       EmojiHighlighter("emoji_heart", true, BrushPaint::SelfOverlap::kAny,
                        EmojiHighlighterVersion::kV1)},
      {"heart_emoji_highlighter_no_trail_1",
       EmojiHighlighter("emoji_heart", false, BrushPaint::SelfOverlap::kAny,
                        EmojiHighlighterVersion::kV1)},
  };
  return families;
}
}  // namespace ink::stock_brushes
