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

#include "ink/storage/internal/jni/brush_family_serialization_jni_helper.h"

#include <jni.h>

#include <cstdint>
#include <string>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/storage/brush.h"
#include "ink/storage/internal/jni/brush_family_serialization_native_helper.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

namespace ink::jni {

using ::ink::TextureBitmapProvider;
using ::ink::jni::AbslStringViewToOptionalJByteArray;
using ::ink::jni::JByteArrayToStdString;
using ::ink::jni::JStringToStdString;
using ::ink::native::ClientTextureIdProviderAndBitmapReceiverFromNativeCallback;
using ::ink::native::TextureBitmapProviderFromNativeArrays;

OnDecodeTextureCallback::OnDecodeTextureCallback(JNIEnv* env,
                                                 absl_nullable jobject callback)
    : env_(env), callback_(callback) {
  if (callback != nullptr) {
    callback_class_ =
        static_cast<jclass>(env_->NewLocalRef(env_->GetObjectClass(callback_)));
    // Can't cache this method lookup because it's on an interface and we
    // don't know what class it will be on in advance.
    on_decode_texture_method_ =
        env_->GetMethodID(callback_class_, "onDecodeTexture",
                          "(Ljava/lang/String;[B)Ljava/lang/String;");
  }
}

const char* absl_nullable OnDecodeTextureCallback::OnDecodeTexture(
    const char* absl_nonnull encoded_id, const int8_t* absl_nonnull bitmap,
    int bitmap_size) {
  if (callback_ == nullptr) {
    return encoded_id;
  }
  jstring encoded_id_jstring = env_->NewStringUTF(encoded_id);
  jbyteArray pixel_data_jarray = AbslStringViewToOptionalJByteArray(
      env_, std::string(reinterpret_cast<const char*>(bitmap), bitmap_size));
  jstring new_id_jstring = static_cast<jstring>(
      env_->CallObjectMethod(callback_, on_decode_texture_method_,
                             encoded_id_jstring, pixel_data_jarray));
  env_->DeleteLocalRef(encoded_id_jstring);
  env_->DeleteLocalRef(pixel_data_jarray);
  if (env_->ExceptionCheck()) {
    // Note that we're not clearing the exception here since we want to raise it
    // as-is later. We're counting on the parsing code bailing out on the first
    // error status encountered.
    return nullptr;
  }
  last_texture_string_ = JStringToStdString(env_, new_id_jstring);
  env_->DeleteLocalRef(new_id_jstring);
  return last_texture_string_.c_str();
}

JniTextureMap::JniTextureMap(JNIEnv* env, jobjectArray texture_map_keys,
                             jobjectArray texture_map_values) {
  if (texture_map_keys == nullptr) {
    ABSL_CHECK_EQ(texture_map_values, nullptr);
    return;
  }
  ABSL_CHECK_NE(texture_map_values, nullptr);
  jsize key_length = env->GetArrayLength(texture_map_keys);
  jsize value_length = env->GetArrayLength(texture_map_values);
  ABSL_CHECK_EQ(key_length, value_length);

  texture_ids_.reserve(key_length);
  bitmaps_.reserve(key_length);
  bitmap_lengths_.reserve(key_length);

  for (jsize i = 0; i < key_length; ++i) {
    jstring j_texture_id =
        static_cast<jstring>(env->GetObjectArrayElement(texture_map_keys, i));
    texture_ids_.push_back(JStringToStdString(env, j_texture_id));
    env->DeleteLocalRef(j_texture_id);

    // We need to explicitly pass length here. Since this is binary data,
    // there might be null bytes in the middle, so trying to find the length
    // with strlen will not work.
    jbyteArray j_png_bytes = static_cast<jbyteArray>(
        env->GetObjectArrayElement(texture_map_values, i));
    bitmaps_.push_back(JByteArrayToStdString(env, j_png_bytes));
    bitmap_lengths_.push_back(bitmaps_.back().length());
    env->DeleteLocalRef(j_png_bytes);
  }

  // c_str doesn't necessarily point to the heap, so taking these pointers
  // needs to wait until after the vector is populated (during population
  // it might be moved).
  texture_ids_ptrs_.reserve(key_length);
  bitmaps_ptrs_.reserve(key_length);
  for (jsize i = 0; i < key_length; ++i) {
    texture_ids_ptrs_.push_back(texture_ids_[i].c_str());
    bitmaps_ptrs_.push_back(bitmaps_[i].c_str());
  }
}

TextureBitmapProvider CreateTextureBitmapProvider(
    JNIEnv* env, absl_nullable jobjectArray texture_map_keys,
    absl_nullable jobjectArray texture_map_values) {
  JniTextureMap texture_map(env, texture_map_keys, texture_map_values);
  return TextureBitmapProviderFromNativeArrays(
      texture_map.TextureIds(), texture_map.Bitmaps(),
      texture_map.BitmapLengths(), texture_map.NumTextures());
}

ClientTextureIdProviderAndBitmapReceiver OnDecodeTextureJniWrapper(
    OnDecodeTextureCallback& on_decode_texture_callback) {
  return ClientTextureIdProviderAndBitmapReceiverFromNativeCallback(
      &on_decode_texture_callback, OnDecodeTextureNativeCallback);
}

const char* absl_nullable OnDecodeTextureNativeCallback(
    void* absl_nonnull jni_decode_texture_callback,
    const char* absl_nonnull encoded_id, const int8_t* absl_nonnull bitmap,
    int bitmap_size) {
  return reinterpret_cast<OnDecodeTextureCallback*>(jni_decode_texture_callback)
      ->OnDecodeTexture(encoded_id, bitmap, bitmap_size);
}

}  // namespace ink::jni
