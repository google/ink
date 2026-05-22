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

#include <jni.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "ink/brush/version.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/storage/brush.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

namespace ink::jni {

using ::ink::TextureBitmapProvider;
using ::ink::jni::AbslStringViewToJByteArray;
using ::ink::jni::JByteArrayToStdString;
using ::ink::jni::JStringToStdString;

TextureBitmapProvider CreateTextureBitmapProvider(
    JNIEnv* env, jobjectArray texture_map_keys,
    jobjectArray texture_map_values) {
  auto texture_map =
      std::make_shared<absl::flat_hash_map<std::string, std::string>>();

  jsize key_length = env->GetArrayLength(texture_map_keys);
  jsize value_length = env->GetArrayLength(texture_map_values);
  ABSL_CHECK_EQ(key_length, value_length);

  for (jsize i = 0; i < key_length; ++i) {
    jstring j_texture_id =
        static_cast<jstring>(env->GetObjectArrayElement(texture_map_keys, i));
    std::string texture_id = JStringToStdString(env, j_texture_id);
    env->DeleteLocalRef(j_texture_id);

    jbyteArray j_png_bytes = static_cast<jbyteArray>(
        env->GetObjectArrayElement(texture_map_values, i));
    std::string png_bytes = JByteArrayToStdString(env, j_png_bytes);
    env->DeleteLocalRef(j_png_bytes);

    texture_map->insert({std::move(texture_id), std::move(png_bytes)});
  }

  return [texture_map](
             absl::string_view texture_id) -> std::optional<std::string> {
    if (auto it = texture_map->find(texture_id); it != texture_map->end()) {
      return it->second;
    }
    return std::nullopt;
  };
}

ClientTextureIdProviderAndBitmapReceiver CreateDecodeTextureJniWrapper(
    JNIEnv* env, absl_nullable jobject callback) {
  if (callback == nullptr) {
    return [](absl::string_view encoded_id,
              absl::string_view bitmap) -> absl::StatusOr<std::string> {
      return std::string(encoded_id);
    };
  }
  jclass callback_class = env->GetObjectClass(callback);
  // Can't cache this method lookup because it's on an interface and we don't
  // know what class it will be on in advance.
  jmethodID on_decode_texture_method =
      env->GetMethodID(callback_class, "onDecodeTexture",
                       "(Ljava/lang/String;[B)Ljava/lang/String;");
  env->DeleteLocalRef(callback_class);

  return [env, callback, on_decode_texture_method](
             absl::string_view encoded_id,
             absl::string_view bitmap) -> absl::StatusOr<std::string> {
    if (env->ExceptionCheck()) {
      return absl::InternalError("Previously encountered exception in JVM.");
    }
    jstring encoded_id_jstring =
        env->NewStringUTF(std::string(encoded_id).c_str());
    jbyteArray pixel_data_jarray =
        bitmap.empty() ? nullptr : AbslStringViewToJByteArray(env, bitmap);

    jstring new_id_jstring = static_cast<jstring>(
        env->CallObjectMethod(callback, on_decode_texture_method,
                              encoded_id_jstring, pixel_data_jarray));
    env->DeleteLocalRef(encoded_id_jstring);
    env->DeleteLocalRef(pixel_data_jarray);

    if (env->ExceptionCheck()) {
      // Note that we're not clearing the exception here since we want to
      // raise it as-is later. We're counting on the parsing code bailing out
      // on the first error status encountered.
      return absl::InternalError("onDecodeTexture raised exception.");
    }
    std::string new_id = JStringToStdString(env, new_id_jstring);
    env->DeleteLocalRef(new_id_jstring);
    return new_id;
  };
}

}  // namespace ink::jni
