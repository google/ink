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

#ifndef INK_ENGINE_SCENE_TYPES_ELEMENT_INDEX_H_
#define INK_ENGINE_SCENE_TYPES_ELEMENT_INDEX_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <list>
#include <utility>
#include <vector>

#include "third_party/absl/container/flat_hash_map.h"
#include "third_party/absl/container/flat_hash_set.h"
#include "third_party/absl/types/optional.h"
#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/log_levels.h"
#include "ink/engine/util/range.h"

namespace ink {

template <typename T, typename H>
class ElementIndex;

// This class is a closure over ElementIndex state for use in std::sort, etc
// from ElementIndex within the context of a single function call.
//
// WARNING -- Do not keep a reference to this class any longer than the
// duration of the sort call or it may blow up!
template <typename IdType, typename Hasher>
class ZIndexSort {
 public:
  bool operator()(const IdType& ei1, const IdType& ei2);

 private:
  explicit ZIndexSort(const ElementIndex<IdType, Hasher>* index);
  const ElementIndex<IdType, Hasher>* index_;

  template <typename T, typename H>
  friend class ElementIndex;
};

// Helper for maintaining a sorting on elements
//
// Mutations are lazy-evaluated. If possible, don't interleave updates/reads
template <typename IdType,
          typename Hasher = typename absl::flat_hash_set<IdType>::hasher>
class ElementIndex {
 public:
  typedef Range<typename std::vector<IdType>::const_iterator> const_iterator;
  typedef Range<typename std::vector<IdType>::const_reverse_iterator>
      const_reverse_iterator;

 public:
  void AddToTop(IdType id);
  void AddToBottom(IdType id);
  void AddBelow(IdType id_to_add, IdType add_below_id);
  void SetBelow(IdType id_to_modify, IdType add_below_id);
  void Remove(IdType id);
  bool Contains(IdType id) const;

  // Returns the ID of the element above the given ID. If the given ID is the
  // topmost element, returns absl::nullopt. The given ID must be one that
  // already exists in the ElementIndex.
  // Note: Normally, the topmost element is considered to be below the "invalid"
  // ID -- however, because IdType could be any type, we don't know what the
  // "invalid" ID is.
  absl::optional<IdType> GetIdAbove(IdType id) const;

  size_t size() const;

  void Clear() {
    id_list_.clear();
    id_to_list_itr_.clear();
    is_dense_id_list_dirty_ = false;
    dense_id_list_cache_.clear();
    is_id_to_zindex_dirty_ = false;
    id_to_zindex_cache_.clear();
  }

  // Iterators are invalid after any non-const call into ElementIndex!
  const_iterator SortedElements() const;
  const_reverse_iterator ReverseSortedElements() const;

  template <class RandomIt>
  void Sort(RandomIt first, RandomIt last) const {
    EnsureDenseIdCache();
    ZIndexSort<IdType, Hasher> sorter(this);
    std::sort(first, last, sorter);
  }

  const absl::flat_hash_map<IdType, uint32_t, Hasher>& IdToZIndexMap() const;
  uint32_t ZIndexOf(IdType id) const;

 private:
  void MarkCacheDirty();
  void EnsureDenseIdCache() const;
  void EnsureIdToZIndexCache() const;

 private:
  // element ids, ordered by z index. Source of truth for ordering
  std::list<IdType> id_list_;

  // element -> idList_ iterator
  absl::flat_hash_map<IdType, typename std::list<IdType>::iterator, Hasher>
      id_to_list_itr_;

  // More efficient, cached versions of "idList_" are lazy updated to avoid
  // n^2 behavior on multiple adds. isDirty[...]_ tracks the cache state
  mutable bool is_dense_id_list_dirty_ = false;
  mutable std::vector<IdType> dense_id_list_cache_;
  mutable bool is_id_to_zindex_dirty_ = false;
  mutable absl::flat_hash_map<IdType, uint32_t, Hasher> id_to_zindex_cache_;

  template <typename T, typename H>
  friend class ZIndexSort;
};

template <typename IdType, typename Hasher>
bool ZIndexSort<IdType, Hasher>::operator()(const IdType& ei1,
                                            const IdType& ei2) {
  ASSERT(index_->id_to_zindex_cache_.count(ei1) > 0);
  ASSERT(index_->id_to_zindex_cache_.count(ei2) > 0);
  return index_->id_to_zindex_cache_[ei1] < index_->id_to_zindex_cache_[ei2];
}

template <typename IdType, typename Hasher>
ZIndexSort<IdType, Hasher>::ZIndexSort(
    const ElementIndex<IdType, Hasher>* index)
    : index_(index) {
  index_->EnsureIdToZIndexCache();
}

///////////////////////////////////////////////////////////////////////////////

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::AddToTop(IdType id) {
  EXPECT(!Contains(id));

  id_list_.push_back(id);
  id_to_list_itr_[id] =
      (--id_list_.end());  // iter to the element we just added

  // Special case -- we don't need to dirty our cache, just add this
  // id to the back of each structure
  dense_id_list_cache_.push_back(id);
  id_to_zindex_cache_[id] = dense_id_list_cache_.size() - 1;
}

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::AddToBottom(IdType id) {
  EXPECT(!Contains(id));

  id_list_.push_front(id);
  id_to_list_itr_[id] = id_list_.begin();  // iter to the element we just added

  MarkCacheDirty();
}

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::AddBelow(IdType id_to_add,
                                            IdType add_below_id) {
  EXPECT(!Contains(id_to_add));

  auto itritr = id_to_list_itr_.find(add_below_id);
  if (itritr == id_to_list_itr_.end()) {
    RUNTIME_ERROR("attempting to add id $0 below unmapped id: $0!", id_to_add,
                  add_below_id);
  }

  auto new_itr = id_list_.insert(itritr->second, id_to_add);
  id_to_list_itr_[id_to_add] = new_itr;
  MarkCacheDirty();
}

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::SetBelow(IdType id_to_modify,
                                            IdType add_below_id) {
  Remove(id_to_modify);
  AddBelow(id_to_modify, add_below_id);
}

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::Remove(IdType id) {
  auto itritr = id_to_list_itr_.find(id);
  if (itritr == id_to_list_itr_.end()) {
    SLOG(SLOG_ERROR, "removing unmapped id: $0!", id);
    return;
  }

  id_list_.erase(itritr->second);
  id_to_list_itr_.erase(itritr);
  MarkCacheDirty();
}

template <typename IdType, typename Hasher>
bool ElementIndex<IdType, Hasher>::Contains(IdType id) const {
  return id_to_list_itr_.find(id) != id_to_list_itr_.end();
}

template <typename IdType, typename Hasher>
absl::optional<IdType> ElementIndex<IdType, Hasher>::GetIdAbove(
    IdType id) const {
  auto map_it = id_to_list_itr_.find(id);
  EXPECT(map_it != id_to_list_itr_.end());
  auto list_it = map_it->second;
  ASSERT(list_it != id_list_.end());
  ++list_it;
  if (list_it == id_list_.end()) return absl::nullopt;
  return *list_it;
}

template <typename IdType, typename Hasher>
size_t ElementIndex<IdType, Hasher>::size() const {
  return id_list_.size();
}

template <typename IdType, typename Hasher>
typename ElementIndex<IdType, Hasher>::const_iterator
ElementIndex<IdType, Hasher>::SortedElements() const {
  EnsureDenseIdCache();
  return const_iterator(dense_id_list_cache_.begin(),
                        dense_id_list_cache_.end());
}

template <typename IdType, typename Hasher>
typename ElementIndex<IdType, Hasher>::const_reverse_iterator
ElementIndex<IdType, Hasher>::ReverseSortedElements() const {
  EnsureDenseIdCache();
  return const_reverse_iterator(dense_id_list_cache_.rbegin(),
                                dense_id_list_cache_.rend());
}

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::MarkCacheDirty() {
  is_dense_id_list_dirty_ = true;
  is_id_to_zindex_dirty_ = true;
}

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::EnsureDenseIdCache() const {
  if (!is_dense_id_list_dirty_) return;
  dense_id_list_cache_.resize(id_list_.size());
  std::copy(id_list_.begin(), id_list_.end(), dense_id_list_cache_.begin());
  is_dense_id_list_dirty_ = false;
}

template <typename IdType, typename Hasher>
void ElementIndex<IdType, Hasher>::EnsureIdToZIndexCache() const {
  if (!is_id_to_zindex_dirty_) return;
  EnsureDenseIdCache();
  id_to_zindex_cache_.clear();
  for (size_t i = 0; i < dense_id_list_cache_.size(); i++) {
    id_to_zindex_cache_[dense_id_list_cache_[i]] = i;
  }
  is_id_to_zindex_dirty_ = false;
}

template <typename IdType, typename Hasher>
const absl::flat_hash_map<IdType, uint32_t, Hasher>&
ElementIndex<IdType, Hasher>::IdToZIndexMap() const {
  EnsureIdToZIndexCache();
  return id_to_zindex_cache_;
}

template <typename IdType, typename Hasher>
uint32_t ElementIndex<IdType, Hasher>::ZIndexOf(IdType id) const {
  EnsureIdToZIndexCache();
  auto itr = id_to_zindex_cache_.find(id);
  if (itr == id_to_zindex_cache_.end()) {
    RUNTIME_ERROR("zindex lookup of unmapped id $0", id);
  }
  return itr->second;
}

}  // namespace ink

#endif  // INK_ENGINE_SCENE_TYPES_ELEMENT_INDEX_H_
