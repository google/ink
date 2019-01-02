// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ink/engine/rendering/gl_managers/client_bitmap_pool.h"

#include "third_party/absl/memory/memory.h"

namespace ink {

ClientBitmapPool::ClientBitmapPool(size_t pool_size,
                                   ::ink::ImageSize image_size,
                                   ImageFormat image_format)
    : data_pool_(std::make_shared<DataPool>()),
      pool_size_(pool_size),
      image_size_(image_size),
      image_format_(image_format),
      bytes_per_bitmap_(image_size_.width * image_size_.height *
                        bytesPerTexelForFormat(image_format_)) {
  data_pool_->block.resize(pool_size * bytes_per_bitmap_, 0);
  for (size_t i = 0; i < pool_size; i++) {
    data_pool_->freelist.push_back(i);
  }
}

ClientBitmapPool::~ClientBitmapPool() {
  SLOG(SLOG_OBJ_LIFETIME,
       "freeing $0 bytes during destruction of client bitmap pool",
       data_pool_->block.size());
}

size_t ClientBitmapPool::FreeCount() const {
  absl::MutexLock lock(&data_pool_->mutex);
  return data_pool_->freelist.size();
}

std::unique_ptr<ClientBitmap> ClientBitmapPool::TakeBitmap() {
  absl::MutexLock lock(&data_pool_->mutex);
  if (data_pool_->freelist.empty()) {
    SLOG(SLOG_WARNING,
         "Taking bitmap from empty pool. Consider increasing the size of this "
         "pool (currently $0).",
         pool_size_);
    return absl::make_unique<RawClientBitmap>(image_size_, image_format_);
  }
  const size_t index = data_pool_->freelist.front();
  data_pool_->freelist.pop_front();
  SLOG(SLOG_OBJ_LIFETIME, "taking bitmap $0", index);
  return absl::make_unique<PoolClientBitmap>(
      data_pool_, image_size_, image_format_,
      data_pool_->block.data() + (index * bytes_per_bitmap_), index);
}

}  // namespace ink
