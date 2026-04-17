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

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_behavior.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/brush/version.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/storage/brush.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

namespace {

using ::ink::Brush;
using ::ink::BrushBehavior;
using ::ink::BrushCoat;
using ::ink::BrushFamily;
using ::ink::BrushPaint;
using ::ink::BrushTip;
using ::ink::ClientTextureIdProviderAndBitmapReceiver;
using ::ink::DecodeBrush;
using ::ink::DecodeBrushBehavior;
using ::ink::DecodeBrushCoat;
using ::ink::DecodeBrushFamily;
using ::ink::DecodeBrushPaint;
using ::ink::DecodeBrushTip;
using ::ink::DecodeMultipleBrushFamilies;
using ::ink::EncodeBrush;
using ::ink::EncodeBrushBehavior;
using ::ink::EncodeBrushCoat;
using ::ink::EncodeBrushFamily;
using ::ink::EncodeBrushPaint;
using ::ink::EncodeBrushTip;
using ::ink::EncodeMultipleBrushFamilies;
using ::ink::TextureBitmapProvider;
using ::ink::Version;
using ::ink::jni::AbslStringViewToJByteArray;
using ::ink::jni::JByteArrayToStdString;
using ::ink::jni::JStringToStdString;
using ::ink::jni::ParseProtoFromEither;
using ::ink::jni::SerializeProto;
using ::ink::jni::ThrowExceptionFromStatus;
using ::ink::native::CastToBrush;
using ::ink::native::CastToBrushBehavior;
using ::ink::native::CastToBrushCoat;
using ::ink::native::CastToBrushFamily;
using ::ink::native::CastToBrushPaint;
using ::ink::native::CastToBrushTip;
using ::ink::native::IntToVersion;
using ::ink::native::NewNativeBrush;
using ::ink::native::NewNativeBrushBehavior;
using ::ink::native::NewNativeBrushCoat;
using ::ink::native::NewNativeBrushFamily;
using ::ink::native::NewNativeBrushPaint;
using ::ink::native::NewNativeBrushTip;

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
    JNIEnv* env, jobject callback) {
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

}  // namespace

extern "C" {

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrush)
(JNIEnv* env, jobject object, jlong brush_native_pointer) {
  ink::proto::Brush brush_proto;
  EncodeBrush(CastToBrush(brush_native_pointer), brush_proto);
  return SerializeProto(env, brush_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushFamily)
(JNIEnv* env, jobject object, jlong brush_family_native_pointer,
 jobjectArray texture_map_keys, jobjectArray texture_map_values) {
  TextureBitmapProvider texture_bitmap_provider =
      CreateTextureBitmapProvider(env, texture_map_keys, texture_map_values);

  ink::proto::BrushFamily brush_family_proto;
  EncodeBrushFamily(CastToBrushFamily(brush_family_native_pointer),
                    brush_family_proto, texture_bitmap_provider);
  return SerializeProto(env, brush_family_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray,
           serializeMultipleBrushFamilies)
(JNIEnv* env, jobject object, jlongArray brush_family_native_pointers,
 jobjectArray texture_map_keys, jobjectArray texture_map_values) {
  TextureBitmapProvider texture_bitmap_provider =
      CreateTextureBitmapProvider(env, texture_map_keys, texture_map_values);

  jsize pointers_length = env->GetArrayLength(brush_family_native_pointers);
  jlong* pointers =
      env->GetLongArrayElements(brush_family_native_pointers, nullptr);
  std::vector<BrushFamily> families;
  families.reserve(pointers_length);
  for (jsize i = 0; i < pointers_length; ++i) {
    families.push_back(CastToBrushFamily(pointers[i]));
  }
  env->ReleaseLongArrayElements(brush_family_native_pointers, pointers,
                                JNI_ABORT);

  ink::proto::BrushFamily brush_family_proto;
  EncodeMultipleBrushFamilies(families, brush_family_proto,
                              texture_bitmap_provider);
  return SerializeProto(env, brush_family_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushCoat)
(JNIEnv* env, jobject object, jlong brush_coat_native_pointer) {
  ink::proto::BrushCoat brush_coat_proto;
  EncodeBrushCoat(CastToBrushCoat(brush_coat_native_pointer), brush_coat_proto);
  return SerializeProto(env, brush_coat_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray,
           serializeBrushBehavior)
(JNIEnv* env, jobject object, jlong brush_behavior_native_pointer) {
  ink::proto::BrushBehavior brush_behavior_proto;
  EncodeBrushBehavior(CastToBrushBehavior(brush_behavior_native_pointer),
                      brush_behavior_proto);
  return SerializeProto(env, brush_behavior_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushTip)
(JNIEnv* env, jobject object, jlong brush_tip_native_pointer) {
  ink::proto::BrushTip brush_tip_proto;
  EncodeBrushTip(CastToBrushTip(brush_tip_native_pointer), brush_tip_proto);
  return SerializeProto(env, brush_tip_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushPaint)
(JNIEnv* env, jobject object, jlong brush_paint_native_pointer) {
  ink::proto::BrushPaint brush_paint_proto;
  EncodeBrushPaint(CastToBrushPaint(brush_paint_native_pointer),
                   brush_paint_proto);
  return SerializeProto(env, brush_paint_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushFromProto)
(JNIEnv* env, jobject object, jobject brush_direct_byte_buffer,
 jbyteArray brush_byte_array, jint offset, jint length,
 jint max_version_value) {
  ink::proto::Brush brush_proto;
  if (absl::Status status =
          ParseProtoFromEither(env, brush_direct_byte_buffer, brush_byte_array,
                               offset, length, brush_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  absl::StatusOr<Version> max_version = IntToVersion(max_version_value);
  if (!max_version.ok()) {
    ThrowExceptionFromStatus(env, max_version.status());
    return 0;
  }
  absl::StatusOr<Brush> brush = DecodeBrush(brush_proto, max_version.value());
  if (!brush.ok()) {
    ThrowExceptionFromStatus(env, brush.status());
    return 0;
  }
  return NewNativeBrush(*std::move(brush));
}

JNI_METHOD(storage, BrushSerializationNative, jlong,
           newBrushFamilyFromProtoInternal)
(JNIEnv* env, jobject object, jobject brush_family_direct_byte_buffer,
 jbyteArray brush_family_byte_array, jint offset, jint length, jobject callback,
 jint max_version_value) {
  ink::proto::BrushFamily brush_family_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_family_direct_byte_buffer, brush_family_byte_array, offset,
          length, brush_family_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }

  ClientTextureIdProviderAndBitmapReceiver decode_texture_jni_wrapper =
      CreateDecodeTextureJniWrapper(env, callback);

  absl::StatusOr<Version> max_version = IntToVersion(max_version_value);
  if (!max_version.ok()) {
    ThrowExceptionFromStatus(env, max_version.status());
    return 0;
  }
  absl::StatusOr<BrushFamily> brush_family = DecodeBrushFamily(
      brush_family_proto, decode_texture_jni_wrapper, max_version.value());
  if (!brush_family.ok()) {
    // If the callback raised an exception we want to raise that as-is
    // instead of replacing it with the status.
    if (!env->ExceptionCheck()) {
      ThrowExceptionFromStatus(env, brush_family.status());
    }
    return 0;
  }
  return NewNativeBrushFamily(*std::move(brush_family));
}

JNI_METHOD(storage, BrushSerializationNative, jlongArray,
           newMultipleBrushFamiliesFromProtoInternal)
(JNIEnv* env, jobject object, jobject brush_family_direct_byte_buffer,
 jbyteArray brush_family_byte_array, jint offset, jint length, jobject callback,
 jint max_version_value) {
  ink::proto::BrushFamily brush_family_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_family_direct_byte_buffer, brush_family_byte_array, offset,
          length, brush_family_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return nullptr;
  }

  ClientTextureIdProviderAndBitmapReceiver decode_texture_jni_wrapper =
      CreateDecodeTextureJniWrapper(env, callback);

  absl::StatusOr<Version> max_version = IntToVersion(max_version_value);
  if (!max_version.ok()) {
    ThrowExceptionFromStatus(env, max_version.status());
    return nullptr;
  }
  absl::StatusOr<std::vector<BrushFamily>> brush_families =
      DecodeMultipleBrushFamilies(
          brush_family_proto, decode_texture_jni_wrapper, max_version.value());
  if (!brush_families.ok()) {
    if (!env->ExceptionCheck()) {
      ThrowExceptionFromStatus(env, brush_families.status());
    }
    return nullptr;
  }

  jlongArray result = env->NewLongArray(brush_families->size());
  jlong* result_pointers = env->GetLongArrayElements(result, nullptr);
  for (size_t i = 0; i < brush_families->size(); ++i) {
    result_pointers[i] = NewNativeBrushFamily((*brush_families)[i]);
  }
  env->ReleaseLongArrayElements(result, result_pointers, 0);
  return result;
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushCoatFromProto)
(JNIEnv* env, jobject object, jobject brush_coat_direct_byte_buffer,
 jbyteArray brush_coat_byte_array, jint offset, jint length) {
  ink::proto::BrushCoat brush_coat_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_coat_direct_byte_buffer, brush_coat_byte_array, offset,
          length, brush_coat_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  absl::StatusOr<BrushCoat> brush_coat = DecodeBrushCoat(brush_coat_proto);
  if (!brush_coat.ok()) {
    ThrowExceptionFromStatus(env, brush_coat.status());
    return 0;
  }
  return NewNativeBrushCoat(*std::move(brush_coat));
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushBehaviorFromProto)
(JNIEnv* env, jobject object, jobject brush_behavior_direct_byte_buffer,
 jbyteArray brush_behavior_byte_array, jint offset, jint length) {
  ink::proto::BrushBehavior brush_behavior_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_behavior_direct_byte_buffer, brush_behavior_byte_array,
          offset, length, brush_behavior_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  absl::StatusOr<BrushBehavior> brush_behavior =
      DecodeBrushBehavior(brush_behavior_proto);
  if (!brush_behavior.ok()) {
    ThrowExceptionFromStatus(env, brush_behavior.status());
    return 0;
  }
  return NewNativeBrushBehavior(*std::move(brush_behavior));
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushTipFromProto)
(JNIEnv* env, jobject object, jobject brush_tip_direct_byte_buffer,
 jbyteArray brush_tip_byte_array, jint offset, jint length) {
  ink::proto::BrushTip brush_tip_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_tip_direct_byte_buffer, brush_tip_byte_array, offset,
          length, brush_tip_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  absl::StatusOr<BrushTip> brush_tip = DecodeBrushTip(brush_tip_proto);
  if (!brush_tip.ok()) {
    ThrowExceptionFromStatus(env, brush_tip.status());
    return 0;
  }
  return NewNativeBrushTip(*std::move(brush_tip));
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushPaintFromProto)
(JNIEnv* env, jobject object, jobject brush_paint_direct_byte_buffer,
 jbyteArray brush_paint_byte_array, jint offset, jint length) {
  ink::proto::BrushPaint brush_paint_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_paint_direct_byte_buffer, brush_paint_byte_array, offset,
          length, brush_paint_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }
  absl::StatusOr<BrushPaint> brush_paint = DecodeBrushPaint(brush_paint_proto);
  if (!brush_paint.ok()) {
    ThrowExceptionFromStatus(env, brush_paint.status());
    return 0;
  }
  return NewNativeBrushPaint(*std::move(brush_paint));
}

}  // extern "C"
