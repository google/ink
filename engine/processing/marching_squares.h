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

#ifndef INK_ENGINE_PROCESSING_MARCHING_SQUARES_H_
#define INK_ENGINE_PROCESSING_MARCHING_SQUARES_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "third_party/glm/glm/glm.hpp"
#include "ink/engine/rendering/baseGL/gpupixels.h"

namespace ink {
namespace marching_squares {

class ColorEqualPredicate {
 public:
  explicit ColorEqualPredicate(uint32_t target_color)
      : target_color_(target_color) {}
  bool operator()(uint32_t value) const { return value == target_color_; }

 private:
  uint32_t target_color_;
};

// Template type Predicate is used to determine which pixels are considered
// "filled". It must provide bool operator()(uint32_t) const.
//
// A note on coordinates: The pixel at index (i, j) is considered to fill the
// square from (i, j) to (i+1, j+1). So, for an M-by-N pixel grid, the indices
// of the pixels lie in [0, M-1]x[0, N-1], but it actually covers the rectangle
// from (0, 0) to (M, N).
template <typename Predicate>
class MarchingSquares {
 public:
  explicit MarchingSquares(GPUPixels* pb) : MarchingSquares(Predicate(), pb) {}
  MarchingSquares(const Predicate& test_predicate, GPUPixels* pb)
      : test_predicate_(test_predicate), pb_(pb) {}

  inline bool CheckBoundary(const glm::ivec2& position) const;
  std::vector<glm::ivec2> TraceBoundary(const glm::ivec2& start_position) const;

  template <typename ForwardIt>
  std::vector<std::vector<glm::ivec2>> TraceBoundaries(
      const ForwardIt& begin, const ForwardIt& end) const;
  std::vector<std::vector<glm::ivec2>> TraceAllBoundaries() const;

 private:
  enum class Direction { N, E, S, W, Unknown };
  Direction NextDirection(const glm::ivec2& position,
                          Direction previous_dir) const;
  bool TestPixel(const glm::ivec2& pixel_index) const;
  uint32_t NeighborPixelState(glm::ivec2 position) const;

  Predicate test_predicate_;
  GPUPixels* pb_;

  // Iterates an NxM grid of glm::ivec2, starting at (0, 0), and continuing by
  // increasing x, then increasing y. The end value will be (0, M).
  class GridIter {
   public:
    using value_type = glm::ivec2;

    explicit GridIter(glm::ivec2 size) : GridIter(size, glm::ivec2(0, 0)) {}
    GridIter(const glm::ivec2& size, const glm::ivec2& position)
        : size_(size), position_(position) {
      ASSERT(size_.x > 0 && size_.y > 0);
    }

    glm::ivec2 operator*() const { return position_; }
    const glm::ivec2* operator->() const { return &position_; }
    GridIter& operator++() {
      ++position_.x;
      if (position_.x == size_.x) {
        position_.x = 0;
        ++position_.y;
      }
      return *this;
    }
    bool operator!=(const GridIter& other) {
      return size_ != other.size_ || position_ != other.position_;
    }

   private:
    glm::ivec2 size_{0, 0};
    glm::ivec2 position_{0, 0};
  };
};

template <typename Predicate>
bool MarchingSquares<Predicate>::CheckBoundary(
    const glm::ivec2& position) const {
  uint32_t neighbors = NeighborPixelState(position);
  return neighbors != 0b0000 && neighbors != 0b1111;
}

template <typename Predicate>
std::vector<glm::ivec2> MarchingSquares<Predicate>::TraceBoundary(
    const glm::ivec2& start_position) const {
  Direction start_dir = NextDirection(start_position, Direction::Unknown);
  if (start_dir == Direction::Unknown) return {};

  std::vector<glm::ivec2> result;
  glm::ivec2 position = start_position;
  Direction next_dir = start_dir;
  int iterations = 0;
  do {
    if (iterations++ > 75000) {
      ASSERT(false);
      result.clear();
      break;
    }

    result.push_back(position);
    if (next_dir == Direction::N)
      position.y++;
    else if (next_dir == Direction::W)
      position.x--;
    else if (next_dir == Direction::S)
      position.y--;
    else if (next_dir == Direction::E)
      position.x++;

    next_dir = NextDirection(position, next_dir);
    EXPECT(next_dir != Direction::Unknown);
  } while (position != start_position || next_dir != start_dir);

  return result;
}

template <typename Predicate>
template <typename ForwardIt>
std::vector<std::vector<glm::ivec2>>
MarchingSquares<Predicate>::TraceBoundaries(const ForwardIt& it_begin,
                                            const ForwardIt& it_end) const {
  static_assert(std::is_same<typename ForwardIt::value_type, glm::ivec2>::value,
                "ForwardIt must have a value_type of glm::ivec2");
  std::vector<std::vector<glm::ivec2>> boundaries;
  glm::ivec2 size = pb_->PixelDim() + glm::ivec2(1, 1);
  std::vector<bool> visited_points(size.x * size.y, 0);
  for (ForwardIt it = it_begin; it != it_end; ++it) {
    int index = it->x + it->y * size.x;
    if (visited_points[index]) continue;
    visited_points[index] = true;
    std::vector<glm::ivec2> boundary = TraceBoundary(*it);
    if (!boundary.empty()) {
      for (const auto& b : boundary) visited_points[b.x + b.y * size.x] = true;
      boundaries.push_back(std::move(boundary));
    }
  }
  return boundaries;
}

template <typename Predicate>
std::vector<std::vector<glm::ivec2>>
MarchingSquares<Predicate>::TraceAllBoundaries() const {
  glm::ivec2 grid_size = pb_->PixelDim() + glm::ivec2(1, 1);
  return TraceBoundaries(GridIter(grid_size),
                         GridIter(grid_size, glm::ivec2(0, grid_size.y)));
}

template <typename Predicate>
bool MarchingSquares<Predicate>::TestPixel(
    const glm::ivec2& pixel_index) const {
  if (!pb_->InBounds(pixel_index)) return false;
  return test_predicate_(pb_->Get(pixel_index));
}

template <typename Predicate>
uint32_t MarchingSquares<Predicate>::NeighborPixelState(
    glm::ivec2 position) const {
  bool down_left = TestPixel(glm::ivec2(position.x - 1, position.y - 1));
  bool down_right = TestPixel(glm::ivec2(position.x, position.y - 1));
  bool up_left = TestPixel(glm::ivec2(position.x - 1, position.y));
  bool up_right = TestPixel(glm::ivec2(position.x, position.y));

  uint32_t state = 0;
  if (down_left) state |= 1;
  if (down_right) state |= 2;
  if (up_right) state |= 4;
  if (up_left) state |= 8;
  return state;
}

template <typename Predicate>
typename MarchingSquares<Predicate>::Direction
MarchingSquares<Predicate>::NextDirection(const glm::ivec2& position,
                                          Direction previous_dir) const {
  uint32_t neighbors = NeighborPixelState(position);
  if (neighbors == 0b0101) {
    if (previous_dir == Direction::N)
      return Direction::W;
    else
      return Direction::E;
  }
  if (neighbors == 0b1010) {
    if (previous_dir == Direction::W)
      return Direction::S;
    else
      return Direction::N;
  }
  if ((neighbors & 0b1100) == 0b1000) return Direction::N;
  if ((neighbors & 0b0110) == 0b0100) return Direction::E;
  if ((neighbors & 0b0011) == 0b0010) return Direction::S;
  if ((neighbors & 0b1001) == 0b0001) return Direction::W;
  return Direction::Unknown;
}

}  // namespace marching_squares
}  // namespace ink

#endif  // INK_ENGINE_PROCESSING_MARCHING_SQUARES_H_
