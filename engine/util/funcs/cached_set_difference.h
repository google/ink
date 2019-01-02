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

#ifndef INK_ENGINE_UTIL_FUNCS_CACHED_SET_DIFFERENCE_H_
#define INK_ENGINE_UTIL_FUNCS_CACHED_SET_DIFFERENCE_H_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

namespace ink {

// Helper class for repeated std::set_difference with caching to prevent
// churning mallocs
template <typename ResultType>
class CachedSetDifference {
 public:
  // useLinearDiffWhenLessThan is used to control whether a linear search (n^2)
  // or sorted merge (n log n) is used to filter.
  explicit CachedSetDifference(size_t use_linear_diff_when_less_than)
      : use_linear_diff_when_less_than_(use_linear_diff_when_less_than) {}
  CachedSetDifference()
      // No scientific basis for the number 40, adjust as profiling dictates.
      : CachedSetDifference(40) {}

  // Returns element_ids that are present in 1, but not in 2.
  // (Same as std::set_difference)
  //
  // The result will stay valid until the next call to filter, callers are
  // free to modify in place. Result is unsorted
  template <typename InputIterator1, typename InputIterator2, class Compare>
  std::vector<ResultType>& Filter(size_t first_size, InputIterator1 first1,
                                  InputIterator1 last1, InputIterator2 first2,
                                  InputIterator2 last2, Compare comp) {
    filter_cache_.clear();

    if (first_size < use_linear_diff_when_less_than_) {
      std::copy_if(first1, last1, std::back_inserter(filter_cache_),
                   [first2, last2](const ResultType& t) {
                     return std::find(first2, last2, t) == last2;
                   });
    } else {
      // The linear search is n^2
      // Fall back to sorting if we have a lot of new elements. (Hardly ever)
      std::vector<ResultType> sorted1(first1, last1);
      std::sort(sorted1.begin(), sorted1.end(), comp);
      std::vector<ResultType> sorted2(first2, last2);
      std::sort(sorted2.begin(), sorted2.end(), comp);

      std::set_difference(sorted1.begin(), sorted1.end(), sorted2.begin(),
                          sorted2.end(), std::back_inserter(filter_cache_),
                          comp);
    }

    return filter_cache_;
  }

 private:
  size_t use_linear_diff_when_less_than_;
  std::vector<ResultType> filter_cache_;
};

}  // namespace ink
#endif  // INK_ENGINE_UTIL_FUNCS_CACHED_SET_DIFFERENCE_H_
