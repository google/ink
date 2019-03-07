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

#ifndef INK_ENGINE_GEOMETRY_SPATIAL_RTREE_H_
#define INK_ENGINE_GEOMETRY_SPATIAL_RTREE_H_

#include <functional>
#include <memory>
#include <vector>

#include "third_party/absl/memory/memory.h"
#include "third_party/absl/types/optional.h"
#include "third_party/absl/types/variant.h"
#include "ink/engine/geometry/algorithms/intersect.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {
namespace spatial {

// An R-Tree data structure for 2D spatial indexing. R-Trees provide efficient
// insertion, deletion, and search operations by organizing the data in a
// self-balancing tree of bounding boxes.
//
// R-Trees do not support updating an element, per se -- instead, elements that
// have changed must be removed and re-inserted.
//
// For more info, see the Wikipedia entry (go/wiki/R-tree) and Guttman's 1984
// paper (http://www-db.deis.unibo.it/courses/SI-LS/papers/Gut84.pdf).
template <typename DataType>
class RTree {
 public:
  // These defaults are taken from the values that we used for boost's R-Tree.
  static constexpr int kDefaultMinChildren = 5;
  static constexpr int kDefaultMaxChildren = 16;

  // This function is used to construct the bounds of an element.
  using BoundsFunction = std::function<Rect(const DataType &)>;

  // This predicate type is used for find and remove operations -- if the
  // predicate returns true, the element will be found or removed.
  using SearchPredicate = std::function<bool(const DataType &)>;

  // Constructs an empty R-Tree.
  // WARNING: The BoundsFunction is saved in the R-Tree; be sure that any
  // references or pointers that it holds remain valid for the lifetime of the
  // R-Tree.
  explicit RTree(BoundsFunction bounds_func,
                 int min_children = kDefaultMinChildren,
                 int max_children = kDefaultMaxChildren);

  // Constructs an R-Tree bulk loaded with the given elements. This will usually
  // result in a better packing of elements (and thus more efficient search)
  // than creating an empty R-Tree and inserting the elements one-by-one.
  // InputIterator must be an iterator over DataType.
  // WARNING: The BoundsFunction is saved in the R-Tree; be sure that any
  // references or pointers that it holds remain valid for the lifetime of the
  // R-Tree.
  template <typename InputIterator>
  RTree(InputIterator begin, InputIterator end, BoundsFunction bounds_func,
        int min_children = kDefaultMinChildren,
        int max_children = kDefaultMaxChildren);

  // Inserts an element into the R-Tree.
  void Insert(const DataType &data);

  // Finds the first element in the traversal, if any, whose bounding boxes
  // intersect the given region, and that match the predicate (if specified).
  // NOTE: Because of the peculiarities of ordering in 2D, and the
  // self-balancing nature of the R-Tree, the traversal order may not be readily
  // apparent, and may change after insertion or deletion.
  // Note that callers can use this API to touch all elements that are in a
  // particular region by providing a predicate that will always return false.
  // This works because the predicate is only called for elements that are in
  // the provided region.
  absl::optional<DataType> FindAny(
      const Rect &region, const SearchPredicate &predicate = nullptr) const;

  // Finds all elements whose bounding box intersects the given region, and that
  // matches the predicate (if specified). Returns the number of elements found.
  // OutputIterator must be an output iterator over DataType.
  template <typename OutputIterator>
  int FindAll(const Rect &region, OutputIterator output,
              const SearchPredicate &predicate = nullptr) const;

  // Removes the first element in the traversal whose bounding box intersects
  // the given region, and that matches the predicate (if specified). Returns
  // true if an element was removed.
  // NOTE: Because of the peculiarities of ordering in 2D, and the
  // self-balancing nature of the R-Tree, the traversal order may not be readily
  // apparent, and may change after insertion or deletion.
  absl::optional<DataType> Remove(const Rect &region,
                                  const SearchPredicate &predicate = nullptr);

  // Removes all elements whose bounding boxes intersect the given region, and
  // that match the predicate (if specified). Returns the number of elements
  // that were removed.
  int RemoveAll(const Rect &region, const SearchPredicate &predicate = nullptr);

  // Clears the R-Tree, removing all elements.
  void Clear();

  // Returns the number of elements in the R-Tree.
  std::size_t Size() const { return n_leaf_nodes_; }

  // Returns the MBR of all of the elements in the R-Tree. If the R-Tree is
  // empty, returns (0, 0)->(0, 0).
  Rect Bounds() const { return root_->Bounds(); }

  // Returns true if this rtree intersects the passed in rtree holding elements
  // of type U. Intersetion means that the elements in the rtrees would
  // intersect.
  // The following must be defined:
  //   geometry::Intersects(const DataType&, const U&) -> bool
  //   geometry::Transform(const DataType&, glm::mat4) -> DataType
  //   geometry::Envelope(const DataType&) -> Rect
  template <typename U>
  bool Intersects(const RTree<U> &other, const glm::mat4 &this_to_other) const;

 private:
  class Node;
  using NodePtrList = std::vector<std::unique_ptr<Node>>;

  class Node {
   public:
    // Constructs an empty branch node at the given level.
    explicit Node(int level);

    // Constructs a leaf node.
    Node(const Rect &bounds, const DataType &data);

    // Constructs a branch node with the given children.
    // InputIterator must be an iterator over Node.
    template <typename InputIterator>
    Node(InputIterator begin, InputIterator end);

    // Adds a child, expanding the bounds if necessary.
    void AddChild(std::unique_ptr<Node> node);

    // Removes the child, transferring ownership to the caller, and recalculates
    // the bounds.
    std::unique_ptr<Node> TakeChild(Node *child);

    // Removes all children, transferring ownership to the caller, and resets
    // the bounds.
    // OutputIterator must be an output iterator over Node.
    template <typename OutputIterator>
    void TakeAllChildren(OutputIterator output);

    int Level() const { return level_; }
    const Rect &Bounds() const { return bounds_; }
    const DataType &Data() const {
      ASSERT(level_ == 0 &&
             absl::holds_alternative<DataType>(data_or_children_));
      return absl::get<DataType>(data_or_children_);
    }
    const NodePtrList &Children() const {
      ASSERT(level_ != 0 &&
             absl::holds_alternative<NodePtrList>(data_or_children_));
      return absl::get<NodePtrList>(data_or_children_);
    }
    Node *Parent() const { return parent_; }

   private:
    // Updates the bounds to the MBR of the children. Calling this on a leaf
    // node does nothing.
    void RecalculateBounds();

    // Enlarges the bounds to include the given rectangle.
    void ExpandBounds(const Rect &bounds);

    // The height of the node in the tree. Leaf nodes are always at level 0.
    int level_;
    // The MBR of the node. For leaf nodes, this is the MBR of its data. For
    // branch nodes, this is the MBR of its children.
    Rect bounds_;
    // The parent of this node. The root's parent will always be nullptr.
    Node *parent_;

    // The data element or children of this node. Leaf nodes will always have
    // data and no children. Branch nodes will always have children and no data.
    absl::variant<DataType, NodePtrList> data_or_children_;
  };

  enum class SearchBehavior { kFindOne, kFindAll };

  void FindLeafNodes(const Node &subtree, const Rect &region,
                     const SearchPredicate &predicate, SearchBehavior behavior,
                     std::vector<Node *> *nodes) const;

  void InsertNode(std::unique_ptr<Node> new_node, Node *subtree);

  // Splits the node, using Guttman's Quadratic Split.
  void SplitNode(Node *node);

  void RemoveLeafNode(Node *node_to_remove);

  float Enlargement(const Rect &base, const Rect &addition) {
    return base.Join(addition).Area() - base.Area();
  }

  // Recursively bulk-loads the R-Tree from the bottom up, using the
  // Sort-Tile-Recursive algorithm
  // (https://archive.org/details/nasa_techdoc_19970016975). Upon return, the
  // nodes vector will contain just a single node, which is the root of the
  // bulk-loaded tree.
  void BulkLoad(NodePtrList *nodes);

  BoundsFunction bounds_func_;
  std::unique_ptr<Node> root_;
  std::size_t n_leaf_nodes_;
  const int min_children_;
  const int max_children_;

  friend class RTreeTest;
};

template <typename DataType>
RTree<DataType>::RTree(BoundsFunction bounds_func, int min_children,
                       int max_children)
    : bounds_func_(bounds_func),
      n_leaf_nodes_(0),
      min_children_(min_children),
      max_children_(max_children) {
  // min_children_ must be sufficiently small w.r.t. max_children_ in order to
  // enforce the bounds on the number of children.
  ASSERT(min_children_ > 0 && 2 * min_children_ <= max_children_ + 1);
  root_ = absl::make_unique<Node>(1);
}

template <typename DataType>
template <typename InputIterator>
RTree<DataType>::RTree(InputIterator begin, InputIterator end,
                       BoundsFunction bounds_func, int min_children,
                       int max_children)
    : RTree(bounds_func, min_children, max_children) {
  n_leaf_nodes_ = std::distance(begin, end);
  if (n_leaf_nodes_ == 0) return;

  NodePtrList nodes;
  nodes.reserve(n_leaf_nodes_);
  for (auto it = begin; it != end; ++it)
    nodes.emplace_back(absl::make_unique<Node>(bounds_func(*it), *it));

  BulkLoad(&nodes);
  if (nodes.front()->Level() == 0) {
    root_ = absl::make_unique<Node>(nodes.begin(), nodes.end());
  } else {
    root_ = std::move(nodes.front());
  }
}

template <typename DataType>
void RTree<DataType>::Insert(const DataType &data) {
  ++n_leaf_nodes_;
  InsertNode(absl::make_unique<Node>(bounds_func_(data), data), root_.get());
}

template <typename DataType>
absl::optional<DataType> RTree<DataType>::Remove(
    const Rect &region, const SearchPredicate &predicate) {
  std::vector<Node *> nodes_to_remove;
  FindLeafNodes(*root_, region, predicate, SearchBehavior::kFindOne,
                &nodes_to_remove);
  ASSERT(nodes_to_remove.size() <= 1);
  absl::optional<DataType> retval = absl::nullopt;
  if (!nodes_to_remove.empty()) {
    retval = nodes_to_remove.front()->Data();
    RemoveLeafNode(nodes_to_remove.front());
    --n_leaf_nodes_;
  }
  return retval;
}

template <typename DataType>
int RTree<DataType>::RemoveAll(const Rect &region,
                               const SearchPredicate &predicate) {
  std::vector<Node *> nodes_to_remove;
  FindLeafNodes(*root_, region, predicate, SearchBehavior::kFindAll,
                &nodes_to_remove);
  for (auto node : nodes_to_remove) RemoveLeafNode(node);
  n_leaf_nodes_ -= nodes_to_remove.size();
  return nodes_to_remove.size();
}

template <typename DataType>
void RTree<DataType>::Clear() {
  root_ = absl::make_unique<Node>(1);
  n_leaf_nodes_ = 0;
}

template <typename DataType>
absl::optional<DataType> RTree<DataType>::FindAny(
    const Rect &region, const SearchPredicate &predicate) const {
  std::vector<Node *> found_nodes;
  FindLeafNodes(*root_, region, predicate, SearchBehavior::kFindOne,
                &found_nodes);
  ASSERT(found_nodes.size() <= 1);
  if (!found_nodes.empty()) {
    return found_nodes.front()->Data();
  } else {
    return absl::nullopt;
  }
}

template <typename DataType>
template <typename OutputIterator>
int RTree<DataType>::FindAll(const Rect &region, OutputIterator output,
                             const SearchPredicate &predicate) const {
  std::vector<Node *> found_nodes;
  FindLeafNodes(*root_, region, predicate, SearchBehavior::kFindAll,
                &found_nodes);
  for (const auto &node : found_nodes) *output++ = node->Data();
  return found_nodes.size();
}

template <typename DataType>
void RTree<DataType>::FindLeafNodes(const Node &subtree, const Rect &region,
                                    const SearchPredicate &predicate,
                                    SearchBehavior behavior,
                                    std::vector<Node *> *nodes) const {
  if (!geometry::Intersects(region, subtree.Bounds())) return;

  if (subtree.Level() == 1) {
    for (const auto &child : subtree.Children()) {
      if (geometry::Intersects(region, child->Bounds()) &&
          (predicate == nullptr || predicate(child->Data()))) {
        nodes->push_back(child.get());
      }
      if (behavior == SearchBehavior::kFindOne && !nodes->empty()) return;
    }
  } else {
    for (const auto &child : subtree.Children()) {
      FindLeafNodes(*child, region, predicate, behavior, nodes);
      if (behavior == SearchBehavior::kFindOne && !nodes->empty()) return;
    }
  }
}

template <typename DataType>
void RTree<DataType>::InsertNode(std::unique_ptr<Node> new_node,
                                 Node *subtree) {
  if (subtree->Level() == new_node->Level() + 1) {
    subtree->AddChild(std::move(new_node));
  } else {
    // Find the child that would be least enlarged by adding the new node. In
    // the event of a tie, choose the one with the smaller area.
    Node *insertion_node = nullptr;
    float min_enlargement = std::numeric_limits<float>::infinity();
    for (const auto &child : subtree->Children()) {
      float enlargement = Enlargement(child->Bounds(), new_node->Bounds());
      if (enlargement < min_enlargement ||
          (enlargement == min_enlargement &&
           (insertion_node == nullptr ||
            child->Bounds().Area() < insertion_node->Bounds().Area()))) {
        min_enlargement = enlargement;
        insertion_node = child.get();
      }
    }
    InsertNode(std::move(new_node), insertion_node);
  }

  if (subtree->Children().size() > max_children_) {
    // The node now has too many children, so we split it into two nodes. If
    // it's the root, we add a new root above it, increasing the tree's
    // height.
    if (subtree->Parent() == nullptr) {
      ASSERT(subtree == root_.get());
      root_ = absl::make_unique<Node>(&root_, &root_ + 1);
    }
    SplitNode(subtree);
  }
}

template <typename DataType>
void RTree<DataType>::SplitNode(Node *node) {
  ASSERT(node->Children().size() == max_children_ + 1);

  NodePtrList children;
  node->TakeAllChildren(std::back_inserter(children));

  // Find the pair of children that would be least efficient in the same node
  // -- they will be the seeds for the new nodes.
  auto left_seed = children.end();
  auto right_seed = children.end();
  float max_inefficiency = -std::numeric_limits<float>::infinity();
  for (auto it1 = ++children.begin(); it1 != children.end(); ++it1) {
    for (auto it2 = children.begin(); it2 != it1; ++it2) {
      float inefficiency = (*it1)->Bounds().Join((*it2)->Bounds()).Area() -
                           (*it1)->Bounds().Area() - (*it2)->Bounds().Area();
      if (inefficiency > max_inefficiency) {
        left_seed = it1;
        right_seed = it2;
        max_inefficiency = inefficiency;
      }
    }
  }
  ASSERT(left_seed != children.end() && right_seed != children.end());

  auto left_node = absl::make_unique<Node>(node->Level());
  left_node->AddChild(std::move(*left_seed));
  children.erase(left_seed);

  auto right_node = absl::make_unique<Node>(node->Level());
  right_node->AddChild(std::move(*right_seed));
  children.erase(right_seed);

  while (!children.empty()) {
    // Determine which child to add next, and which node to add it to.
    auto next_child = children.end();
    Node *node_to_add_to = nullptr;
    if (left_node->Children().size() + children.size() == min_children_) {
      // The remaining children must go to the left node in order to meet the
      // minimum number of children.
      next_child = children.begin();
      node_to_add_to = left_node.get();
    } else if (right_node->Children().size() + children.size() ==
               min_children_) {
      // The remaining children must go to the right node in order to meet the
      // minimum number of children.
      next_child = children.begin();
      node_to_add_to = right_node.get();
    } else {
      // Find the child that most prefers the left or right node, i.e. it
      // would result in the largest difference in node enlargement.
      float max_preference = 0;
      for (auto it = children.begin(); it != children.end(); ++it) {
        float preference = Enlargement(left_node->Bounds(), (*it)->Bounds()) -
                           Enlargement(right_node->Bounds(), (*it)->Bounds());
        if (next_child == children.end() ||
            std::abs(preference) > std::abs(max_preference)) {
          next_child = it;
          max_preference = preference;
        }
      }

      node_to_add_to = max_preference > 0 ? right_node.get() : left_node.get();
    }

    ASSERT(next_child != children.end() && node_to_add_to != nullptr);
    node_to_add_to->AddChild(std::move(*next_child));
    children.erase(next_child);
  }

  node->Parent()->AddChild(std::move(left_node));
  node->Parent()->AddChild(std::move(right_node));
  node->Parent()->TakeChild(node);  // This frees the node.
}

template <typename DataType>
void RTree<DataType>::RemoveLeafNode(Node *node_to_remove) {
  ASSERT(node_to_remove->Level() == 0);

  // Remove the target node, and move upwards through the tree, removing any
  // nodes that no longer meet the minimum number of children. Note also that
  // the root does not need honor to this minimum.
  NodePtrList nodes_to_reinsert;
  Node *node = node_to_remove;
  do {
    Node *parent = node->Parent();
    if (node->Level() > 0)
      // This is a branch node, it's remaining children must be reinserted.
      node->TakeAllChildren(std::back_inserter(nodes_to_reinsert));
    parent->TakeChild(node);  // This frees the node.
    node = parent;
  } while (node != root_.get() && node->Children().size() < min_children_);

  for (auto &node : nodes_to_reinsert) InsertNode(std::move(node), root_.get());

  if (root_->Level() > 1 && root_->Children().size() == 1)
    // The root has only one child, so we can shorten the tree.
    root_ = root_->TakeChild(root_->Children().front().get());
}

template <typename DataType>
void RTree<DataType>::BulkLoad(NodePtrList *nodes) {
  ASSERT(!nodes->empty());
  if (nodes->size() == 1) return;

  auto compare_x = [](const std::unique_ptr<Node> &lhs,
                      const std::unique_ptr<Node> &rhs) {
    return lhs->Bounds().from.x < rhs->Bounds().from.x;
  };
  auto compare_y = [](const std::unique_ptr<Node> &lhs,
                      const std::unique_ptr<Node> &rhs) {
    return lhs->Bounds().from.y < rhs->Bounds().from.y;
  };

  std::sort(nodes->begin(), nodes->end(), compare_x);

  // Divide the nodes horizontally into slices.
  int n_slices =
      std::ceil(std::sqrt(static_cast<float>(nodes->size()) / max_children_));
  int nodes_per_slice =
      std::floor(static_cast<float>(nodes->size()) / n_slices);
  int slice_leftovers = nodes->size() - n_slices * nodes_per_slice;

  int node_idx = 0;
  auto slice_begin = nodes->begin();
  for (int i = 0; i < n_slices; ++i) {
    auto slice_end = slice_begin + (i < slice_leftovers ? nodes_per_slice + 1
                                                        : nodes_per_slice);
    std::sort(slice_begin, slice_end, compare_y);

    // Divide the slice vertically into tiles.
    int slice_size = std::distance(slice_begin, slice_end);
    int n_tiles = std::ceil(static_cast<float>(slice_size) / max_children_);
    int nodes_per_tile = std::floor(static_cast<float>(slice_size) / n_tiles);
    int tile_leftovers = slice_size - n_tiles * nodes_per_tile;
    auto tile_begin = slice_begin;
    for (int j = 0; j < n_tiles; ++j) {
      auto tile_end = tile_begin + (j < tile_leftovers ? nodes_per_tile + 1
                                                       : nodes_per_tile);
      ASSERT(tile_begin < nodes->end());
      ASSERT(tile_end <= nodes->end());

      // We reuse the space in the input vector -- this is safe because the Node
      // constructor will always move elements into the new node faster than we
      // replace the elements in the input vector.
      (*nodes)[node_idx] = absl::make_unique<Node>(tile_begin, tile_end);
      ++node_idx;
      tile_begin = tile_end;
    }
    slice_begin = slice_end;
  }

  // Shrink the nodes vector to fit the new nodes, and repeat.
  ASSERT((*nodes)[node_idx - 1] != nullptr && (*nodes)[node_idx] == nullptr);
  nodes->resize(node_idx);
  BulkLoad(nodes);
}

template <typename DataType>
RTree<DataType>::Node::Node(int level)
    : level_(level),
      bounds_(0, 0, 0, 0),
      parent_(nullptr),
      data_or_children_(absl::in_place_type_t<NodePtrList>()) {
  ASSERT(level > 0);
}

template <typename DataType>
RTree<DataType>::Node::Node(const Rect &bounds, const DataType &data)
    : level_(0), bounds_(bounds), parent_(nullptr), data_or_children_(data) {}

template <typename DataType>
template <typename InputIterator>
RTree<DataType>::Node::Node(InputIterator begin, InputIterator end)
    : level_((*begin)->Level() + 1),
      parent_(nullptr),
      data_or_children_(absl::in_place_type_t<NodePtrList>()) {
  absl::get<NodePtrList>(data_or_children_).reserve(std::distance(begin, end));
  for (auto it = begin; it != end; ++it) AddChild(std::move(*it));
}

template <typename DataType>
void RTree<DataType>::Node::AddChild(std::unique_ptr<Node> node) {
  ASSERT(node->Level() == Level() - 1);
  ASSERT(Level() != 0 &&
         absl::holds_alternative<NodePtrList>(data_or_children_));
  ExpandBounds(node->Bounds());
  node->parent_ = this;
  absl::get<NodePtrList>(data_or_children_).emplace_back(std::move(node));
}

template <typename DataType>
std::unique_ptr<typename RTree<DataType>::Node>
RTree<DataType>::Node::TakeChild(Node *child) {
  ASSERT(child->parent_ == this);
  ASSERT(Level() != 0 &&
         absl::holds_alternative<NodePtrList>(data_or_children_));
  auto &children = absl::get<NodePtrList>(data_or_children_);

  // We are doing a linear search through the children here, but we expect
  // max_children_ to be relatively small (say, less than 50), so it's
  // unlikely to affect performance.
  for (auto it = children.begin(); it != children.end(); ++it) {
    if (it->get() == child) {
      std::unique_ptr<Node> retval = std::move(*it);
      child->parent_ = nullptr;
      children.erase(it);
      RecalculateBounds();
      return retval;
    }
  }
  ASSERT(false);
  return nullptr;
}

template <typename DataType>
template <typename OutputIterator>
void RTree<DataType>::Node::TakeAllChildren(OutputIterator output) {
  ASSERT(Level() != 0 &&
         absl::holds_alternative<NodePtrList>(data_or_children_));
  auto &children = absl::get<NodePtrList>(data_or_children_);
  for (const auto &child : children) child->parent_ = nullptr;
  std::move(children.begin(), children.end(), output);
  children.clear();
  RecalculateBounds();
}

template <typename DataType>
void RTree<DataType>::Node::RecalculateBounds() {
  if (level_ == 0) return;
  Rect old_bounds = Bounds();
  if (Children().empty()) {
    bounds_ = Rect(0, 0, 0, 0);
  } else {
    bounds_ = Children().front()->Bounds();
    for (auto it = ++Children().begin(); it != Children().end(); ++it)
      bounds_ = bounds_.Join((*it)->Bounds());
  }
  if (Parent() != nullptr && old_bounds != Bounds())
    Parent()->RecalculateBounds();
}

template <typename DataType>
void RTree<DataType>::Node::ExpandBounds(const Rect &bounds) {
  Rect old_bounds = Bounds();
  if (Children().empty()) {
    bounds_ = bounds;
  } else {
    bounds_ = bounds_.Join(bounds);
  }
  if (Parent() != nullptr && old_bounds != Bounds())
    Parent()->RecalculateBounds();
}

template <typename DataType>
template <typename U>
bool RTree<DataType>::Intersects(const RTree<U> &other,
                                 const glm::mat4 &this_to_other) const {
  // This can be optimized if we could walk the node subtrees of *this and
  // *other at the same time; such that we do subtree - subtree comparisons
  // instead of subtree- whole tree comparisons as we do now.
  glm::mat4 other_to_this = glm::inverse(this_to_other);
  Rect other_bounds_in_this =
      geometry::Transform(other.Bounds(), other_to_this);
  Rect intersection_in_this;
  if (!geometry::Intersection(Bounds(), other_bounds_in_this,
                              &intersection_in_this)) {
    return false;
  }

  return FindAny(intersection_in_this,
                 [&this_to_other, &other](const DataType &this_data_in_this) {
                   auto this_data_in_other =
                       geometry::Transform(this_data_in_this, this_to_other);
                   return other
                       .FindAny(
                           geometry::Envelope(this_data_in_other),
                           [&this_data_in_other](const U &other_data_in_other) {
                             return geometry::Intersects(this_data_in_other,
                                                         other_data_in_other);
                           })
                       .has_value();
                 })
      .has_value();
}

}  // namespace spatial
}  // namespace ink

#endif  // INK_ENGINE_GEOMETRY_SPATIAL_RTREE_H_
