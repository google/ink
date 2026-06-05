// Copyright 2025 Google LLC
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

#ifndef INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_SERIALIZATION_JNI_HELPER_H_
#define INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_SERIALIZATION_JNI_HELPER_H_

#include <jni.h>

#include <cstdint>
#include <string>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "ink/storage/brush.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

namespace ink::jni {

class OnDecodeTextureCallback {
 public:
  OnDecodeTextureCallback(JNIEnv* env, absl_nullable jobject callback);

  OnDecodeTextureCallback(const OnDecodeTextureCallback&) = delete;
  OnDecodeTextureCallback& operator=(const OnDecodeTextureCallback&) = delete;
  OnDecodeTextureCallback(OnDecodeTextureCallback&&) = delete;
  OnDecodeTextureCallback& operator=(OnDecodeTextureCallback&&) = delete;

  // The returned pointer is valid until the next call to this operator or the
  // destruction of this object. This allows the caller to pass through a
  // pure-C interface to the KMP-common native implementation while the pointer
  // stays in scope. Returns nullptr if an exception is thrown by the JVM
  // callback. This should not be called directly, it should be called via
  // OnDecodeTextureNativeCallback which provides the C-compatible interface
  // to be used along with a pass-through pointer to this object from
  // KMP-compatible native code.
  const char* absl_nullable OnDecodeTexture(const char* absl_nonnull encoded_id,
                                            const int8_t* absl_nonnull bitmap,
                                            int bitmap_size);

 private:
  JNIEnv* env_;
  absl_nullable jobject callback_;
  jclass callback_class_;
  jmethodID on_decode_texture_method_;
  std::string last_texture_string_;
};

class JniTextureMap {
 public:
  JniTextureMap(JNIEnv* env, absl_nullable jobjectArray texture_map_keys,
                absl_nullable jobjectArray texture_map_values);

  const char* absl_nonnull* absl_nonnull TextureIds() {
    return texture_ids_ptrs_.data();
  }
  const int8_t* absl_nullable* absl_nonnull Bitmaps() {
    return reinterpret_cast<const int8_t**>(bitmaps_ptrs_.data());
  }
  const int* absl_nonnull BitmapLengths() { return bitmap_lengths_.data(); }
  int NumTextures() {
    ABSL_CHECK_EQ(texture_ids_.size(), bitmaps_.size());
    ABSL_CHECK_EQ(texture_ids_.size(), texture_ids_ptrs_.size());
    ABSL_CHECK_EQ(bitmaps_.size(), bitmaps_ptrs_.size());
    ABSL_CHECK_EQ(texture_ids_.size(), bitmap_lengths_.size());
    return texture_ids_.size();
  }

 private:
  std::vector<std::string> texture_ids_;
  std::vector<const char* absl_nonnull> texture_ids_ptrs_;
  std::vector<std::string> bitmaps_;
  std::vector<const char* absl_nonnull> bitmaps_ptrs_;
  std::vector<int> bitmap_lengths_;
};

TextureBitmapProvider CreateTextureBitmapProvider(
    JNIEnv* env, absl_nullable jobjectArray texture_map_keys,
    absl_nullable jobjectArray texture_map_values);

ClientTextureIdProviderAndBitmapReceiver OnDecodeTextureJniWrapper(
    OnDecodeTextureCallback& on_decode_texture_callback);

// This also should not be called directly, it should be wrapped in a lambda
// suitable for passing to Ink's DecodeBrushFamily or similar by
// ink::native::ClientTextureIdProviderAndBitmapReceiverFromNativeCallback.
// Returns the passed in encoded_id for the default behavior, so that must be
// from something that outlives the span of the call. Returns nullptr if the
// JVM callback throws an exception.
const char* absl_nullable OnDecodeTextureNativeCallback(
    void* absl_nonnull jni_decode_texture_callback,
    const char* absl_nonnull encoded_id, const int8_t* absl_nonnull bitmap,
    int bitmap_size);

}  // namespace ink::jni

#endif  // INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_SERIALIZATION_JNI_HELPER_H_
