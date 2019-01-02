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

#ifndef INK_ENGINE_SCENE_DATA_COMMON_MRUCACHE_H_
#define INK_ENGINE_SCENE_DATA_COMMON_MRUCACHE_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "ink/engine/util/dbg/errors.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/time/time_types.h"
#include "ink/engine/util/time/wall_clock.h"

namespace ink {

template <class TKey, class TValue>
class MRUCache {
 public:
  typedef TKey key_type;
  typedef TValue mapped_type;
  typedef typename std::unordered_map<key_type, mapped_type>::iterator iterator;
  typedef typename std::unordered_map<key_type, mapped_type>::const_iterator
      const_iterator;

 public:
  uint32_t accesses_before_update_ =
      1;  // ex 1 == always update last access time, 10
          // == update last access time every 10th time
          // the key is touched
  bool track_access_times_ = false;

 public:
  explicit MRUCache(std::shared_ptr<WallClockInterface> wall_clock)
      : wall_clock_(std::move(wall_clock)) {}

  mapped_type& operator[](const key_type& k) {
    auto& v = data_[k];
    v.key = k;
    v.naccesses++;
    WallTimeS currenttime = wall_clock_->CurrentTime();

    if (v.last_access_time.time == WallTimeS(0)) {  // inserting a new item
      v.last_access_time = AccessTime(currenttime, k);
      times_.insert(v.last_access_time);
    } else if (track_access_times_) {  // updating an old item
      if (v.naccesses >= accesses_before_update_) {
        times_.erase(v.last_access_time);
        v.last_access_time.time = currenttime;
        times_.insert(v.last_access_time);
        v.naccesses = 0;
      }
    }

    EXPECT(data_.size() == times_.size());
    return v.value;
  }

  bool Contains(const key_type& key) { return data_.count(key) > 0; }

  void Erase(const key_type& k) {
    auto ai = data_.find(k);
    if (ai == data_.end()) return;
    times_.erase(ai->second.last_access_time);
    data_.erase(k);
    EXPECT(data_.size() == times_.size());
  }

  void Clear() {
    data_.clear();
    times_.clear();
  }

  size_t size() const {
    EXPECT(data_.size() == times_.size());
    return data_.size();
  }

  uint32_t DropCapacity(float percent_to_drop) {
    track_access_times_ = true;
    uint32_t ntodrop = std::min((size_t)(size() * percent_to_drop), size());

    SLOG(SLOG_INFO, "dropping capacity -- $0%, $1 items.",
         (percent_to_drop * 100.0), ntodrop);

    if (ntodrop < 800) {
      SLOG(SLOG_INFO, "not enough capacity to drop, giving up");
      return 0;
    }

    std::vector<key_type> keys_to_drop(ntodrop);
    auto at = times_.rbegin();
    for (uint32_t i = 0; i < ntodrop; i++) {
      EXPECT(at != times_.rend());
      keys_to_drop[i] = at->key;
      at++;
    }

    for (auto ai = keys_to_drop.begin(); ai != keys_to_drop.end(); ai++) {
      Erase(*ai);
    }

    return ntodrop;
  }

 private:
  struct AccessTime {
    WallTimeS time;
    key_type key;

    AccessTime(WallTimeS time, key_type key) : time(time), key(key) {}

    bool operator<(const AccessTime& rhs) const {
      if (time == rhs.time) {
        return key < rhs.key;
      }
      return time < rhs.time;
    }
  };

  struct MRUData {
    key_type key;
    mapped_type value;
    uint32_t naccesses;
    AccessTime last_access_time;

    MRUData() : key(0), naccesses(0), last_access_time(WallTimeS(0), 0) {}
  };

  std::unordered_map<key_type, MRUData> data_;
  std::set<AccessTime> times_;

  std::shared_ptr<WallClockInterface> wall_clock_;

  friend class MRUCacheTestHelper;
};

}  // namespace ink
#endif  // INK_ENGINE_SCENE_DATA_COMMON_MRUCACHE_H_
