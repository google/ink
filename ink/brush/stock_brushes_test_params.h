#ifndef INK_BRUSH_STOCK_BRUSHES_TEST_PARAMS_H_
#define INK_BRUSH_STOCK_BRUSHES_TEST_PARAMS_H_

#include <string>
#include <utility>
#include <vector>

#include "ink/brush/brush_family.h"

namespace ink::stock_brushes {

/* For testing purposes, it is convenient to have a named parameter for
 * each stock brush. As stock brushes are added or updated, this function will
 * need to be updated.
 */
using StockBrushesTestParam = std::pair<std::string, BrushFamily>;
std::vector<stock_brushes::StockBrushesTestParam> GetParams();
}  // namespace ink::stock_brushes

#endif  // INK_BRUSH_STOCK_BRUSHES_TEST_PARAMS_H_
