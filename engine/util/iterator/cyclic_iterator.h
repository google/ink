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

#ifndef INK_ENGINE_UTIL_ITERATOR_CYCLIC_ITERATOR_H_
#define INK_ENGINE_UTIL_ITERATOR_CYCLIC_ITERATOR_H_

#include <iterator>
#include <type_traits>

namespace ink {

// This class is an iterator adapter that allows you to treat a range as cyclic;
// that is, incrementing the last element brings you back to the beginning,
// rather than to the past-the-end iterator.
// Any operation that invalidates the base iterator type also invalidates the
// CyclicIterator.
// Note: The number of cycles that an iterator has performed is not taken into
// account when checking for equality -- that is, for a range with N elements,
// the statement iter == std::next(iter, N) will always be true.
template <typename ForwardIt>
class CyclicIterator {
 public:
  using value_type = typename std::iterator_traits<ForwardIt>::value_type;
  using difference_type =
      typename std::iterator_traits<ForwardIt>::difference_type;
  using reference = typename std::iterator_traits<ForwardIt>::reference;
  using pointer = typename std::iterator_traits<ForwardIt>::pointer;
  using iterator_category = std::bidirectional_iterator_tag;

  static_assert(
      std::is_base_of<std::bidirectional_iterator_tag,
                      iterator_category>::value,
      "CyclicIterator's base iterator type must be a bidirectional iterator");

  // You can use these constructors, but you'll likely find the
  // MakeCyclicIterator() functions easier.
  CyclicIterator() {}
  CyclicIterator(const ForwardIt &begin, const ForwardIt &end,
                 const ForwardIt current_position)
      : begin_(begin), end_(end), current_position_(current_position) {}

  ForwardIt BaseBegin() const { return begin_; }
  ForwardIt BaseEnd() const { return end_; }
  ForwardIt BaseCurrent() const { return current_position_; }

  reference operator*() const { return *current_position_; }
  pointer operator->() const { return &*current_position_; }

  CyclicIterator &operator++() {
    ++current_position_;
    if (current_position_ == end_) current_position_ = begin_;
    return *this;
  }
  CyclicIterator operator++(int) {
    auto retval = *this;
    ++*this;
    return retval;
  }
  CyclicIterator &operator--() {
    if (current_position_ == begin_) current_position_ = end_;
    --current_position_;
    return *this;
  }
  CyclicIterator operator--(int) {
    auto retval = *this;
    --*this;
    return retval;
  }

  bool operator==(const CyclicIterator &other) const {
    return current_position_ == other.current_position_ &&
           begin_ == other.begin_ && end_ == other.end_;
  }

  bool operator!=(const CyclicIterator &other) const {
    return !(*this == other);
  }

 private:
  ForwardIt begin_;
  ForwardIt end_;
  ForwardIt current_position_;
};

// Convenience functions to perform template type deduction.
template <typename ForwardIt>
CyclicIterator<ForwardIt> MakeCyclicIterator(const ForwardIt &begin,
                                             const ForwardIt &end) {
  return CyclicIterator<ForwardIt>(begin, end, begin);
}
template <typename ForwardIt>
CyclicIterator<ForwardIt> MakeCyclicIterator(
    const ForwardIt &begin, const ForwardIt &end,
    const ForwardIt &current_position) {
  return CyclicIterator<ForwardIt>(begin, end, current_position);
}

}  // namespace ink

#endif  // INK_ENGINE_UTIL_ITERATOR_CYCLIC_ITERATOR_H_
