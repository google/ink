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

#ifndef INK_ENGINE_UTIL_RANGE_H_
#define INK_ENGINE_UTIL_RANGE_H_

#include <type_traits>
#include <vector>

namespace ink {

// Helper class for iterating over a range. Owns a copy of a begin and end
// iterator.
//
// Like a boost::range, but simpler.
// (http://www.boost.org/doc/libs/1_60_0/libs/range/doc/html/range/concepts/overview.html)
template <typename TIterator>
class Range {
 public:
  Range(TIterator begin, TIterator end) : begin_(begin), end_(end) {}

  TIterator begin() const { return begin_; }
  TIterator end() const { return end_; }

  // Collapses this range to a std::vector<const T*>, where T is a *TIterator
  template <typename T>
  std::vector<const T*> AsPointerVector() const {
    std::vector<const T*> res;
    for (auto ai = begin_; ai != end_; ai++) {
      res.emplace_back(&(*ai));
    }
    return res;
  }

  // Collapses this range to a std::vector<T>, where T is a *TIterator
  template <typename T>
  std::vector<T> AsValueVector() const {
    std::vector<T> res;
    for (auto ai = begin_; ai != end_; ai++) {
      res.emplace_back(*ai);
    }
    return res;
  }

  decltype(std::distance(std::declval<TIterator>(), std::declval<TIterator>()))
  size() {
    return std::distance(begin_, end_);
  }

 private:
  TIterator begin_;
  TIterator end_;
};

template <typename T>
Range<T*> MakePointerRange(T* t) {
  return Range<T*>(t, t + 1);
}

template <typename T>
Range<typename T::const_iterator> MakeSTLRange(const T& t) {
  return {t.begin(), t.end()};
}

template <typename TIterator>
Range<TIterator> MakeRange(TIterator begin, TIterator end) {
  return {begin, end};
}

}  // namespace ink

#endif  // INK_ENGINE_UTIL_RANGE_H_
