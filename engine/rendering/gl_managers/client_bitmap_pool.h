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

#ifndef INK_ENGINE_RENDERING_GL_MANAGERS_CLIENT_BITMAP_POOL_H_
#define INK_ENGINE_RENDERING_GL_MANAGERS_CLIENT_BITMAP_POOL_H_

#include <list>
#include <memory>
#include <vector>

#include "third_party/absl/synchronization/mutex.h"
#include "ink/engine/public/types/client_bitmap.h"
#include "ink/engine/util/dbg/log.h"

namespace ink {

/**
 * ClientBitmapPool manages a contiguous block of pool_size chunks of RAM.
 * TakeBitmap() provides a ClientBitmap of the configured format and image size,
 * whose destructor automatically returns it to the pool.
 *
 * If a bitmap is taken when the pool is empty, ClientBitmapPool logs a warning
 * and allocates a RawClientBitmap.
 *
 * ClientBitmapPool is threadsafe. The zoom-by-tile design has tiles taken on
 * the task thread, and returned on the GL thread.
 */
class ClientBitmapPool {
 public:
  ClientBitmapPool(size_t pool_size, ::ink::ImageSize image_size,
                   ImageFormat image_format);
  ~ClientBitmapPool();

  // Get a client bitmap from this pool. It will return its storage to the pool
  // when it is destructed.
  std::unique_ptr<ClientBitmap> TakeBitmap();

  // How many bitmaps remain untaken in the pool? Useful mainly for testing,
  // since the client shouldn't care.
  size_t FreeCount() const;

  const ::ink::ImageSize& ImageSize() const { return image_size_; }

  // ClientBitmapPool is neither copyable nor movable.
  ClientBitmapPool(const ClientBitmapPool&) = delete;
  ClientBitmapPool& operator=(const ClientBitmapPool&) = delete;

 private:
  struct DataPool {
    // The single contiguous block of memory owned by this pool.
    std::vector<uint8_t> block;

    // Guard the freelist_.
    mutable absl::Mutex mutex;

    // A list of indexes into the block.
    // An index of N represents byte # (N * bytes_per_bitmap_).
    std::list<size_t> freelist GUARDED_BY(mutex);
  };

  class PoolClientBitmap : public ClientBitmap {
   public:
    PoolClientBitmap(std::shared_ptr<DataPool> data_pool, ::ink::ImageSize size,
                     ImageFormat format, uint8_t* data, std::size_t index)
        : ClientBitmap(size, format),
          data_pool_(std::move(data_pool)),
          data_ptr_(data),
          index_(index) {}

    ~PoolClientBitmap() override {
      absl::MutexLock mu(&data_pool_->mutex);
      SLOG(SLOG_OBJ_LIFETIME, "returning bitmap $0", index_);
      data_pool_->freelist.push_back(index_);
    }
    const void* imageByteData() const override { return data_ptr_; }
    void* imageByteData() override { return data_ptr_; }

   private:
    // The data pool that this bitmap is allocated from. We hold on to this
    // shared pointer in order to keep the data pool alive.
    std::shared_ptr<DataPool> data_pool_;

    // Pointer to this bitmap's data in the data pool.
    uint8_t* data_ptr_;

    // The index of the data in the data pool.
    std::size_t index_;
  };

  std::shared_ptr<DataPool> data_pool_;

  // Size of pool in number of bitmaps.
  const size_t pool_size_;

  // Configuration of bitmaps provided by this pool.
  const ::ink::ImageSize image_size_;
  const ImageFormat image_format_;

  // How much RAM does each bitmap use?
  const size_t bytes_per_bitmap_;
};

}  // namespace ink

#endif  // INK_ENGINE_RENDERING_GL_MANAGERS_CLIENT_BITMAP_POOL_H_
