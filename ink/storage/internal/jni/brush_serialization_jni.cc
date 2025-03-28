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

#include <map>
#include <optional>
#include <string>
#include <utility>

#include "absl/base/nullability.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/jni/internal/jni_string_util.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/storage/brush.h"
#include "ink/storage/proto/brush.pb.h"

namespace {

using ::ink::Brush;
using ::ink::BrushCoat;
using ::ink::BrushFamily;
using ::ink::BrushPaint;
using ::ink::BrushTip;
using ::ink::DecodeBrush;
using ::ink::DecodeBrushCoat;
using ::ink::DecodeBrushFamily;
using ::ink::DecodeBrushPaint;
using ::ink::DecodeBrushTip;
using ::ink::EncodeBrush;
using ::ink::EncodeBrushCoat;
using ::ink::EncodeBrushFamily;
using ::ink::EncodeBrushPaint;
using ::ink::EncodeBrushTip;
using ::ink::jni::JByteArrayToStdString;
using ::ink::jni::JStringToStdString;
using ::ink::jni::ParseProtoFromEither;
using ::ink::jni::SerializeProto;
using ::ink::jni::StdStringToJbyteArray;
using ::ink::jni::ThrowExceptionFromStatus;

}  // namespace

extern "C" {

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrush)
(JNIEnv* env, jobject object, jlong brush_native_pointer) {
  const auto* brush = reinterpret_cast<const Brush*>(brush_native_pointer);
  ink::proto::Brush brush_proto;
  EncodeBrush(*brush, brush_proto);
  return SerializeProto(env, brush_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushFamily)
(JNIEnv* env, jobject object, jlong brush_family_native_pointer,
 jobjectArray texture_map_keys, jobjectArray texture_map_values) {
  const auto* brush_family =
      reinterpret_cast<const BrushFamily*>(brush_family_native_pointer);
  std::map<std::string, std::string> texture_map = {};

  jsize key_length = env->GetArrayLength(texture_map_keys);
  jsize value_length = env->GetArrayLength(texture_map_values);
  if (key_length != value_length) {
    ink::jni::ThrowException(
        env, "java/lang/IllegalArgumentException",
        "textureMapKeys and textureMapValues must have the same length");
    return nullptr;
  }

  for (jsize i = 0; i < key_length; ++i) {
    jstring j_texture_id =
        static_cast<jstring>(env->GetObjectArrayElement(texture_map_keys, i));
    std::string texture_id = JStringToStdString(env, j_texture_id);

    jbyteArray j_png_bytes = static_cast<jbyteArray>(
        env->GetObjectArrayElement(texture_map_values, i));
    std::string png_bytes = JByteArrayToStdString(env, j_png_bytes);
    env->DeleteLocalRef(j_png_bytes);

    texture_map.insert({texture_id, png_bytes});
  }
  ink::TextureBitmapProvider texture_bitmap_provider =
      [&texture_map](
          const std::string& texture_id) -> std::optional<std::string> {
    return texture_map[texture_id];
  };
  ink::proto::BrushFamily brush_family_proto;
  EncodeBrushFamily(*brush_family, brush_family_proto, texture_bitmap_provider);
  return SerializeProto(env, brush_family_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushCoat)
(JNIEnv* env, jobject object, jlong brush_coat_native_pointer) {
  const auto* brush_coat =
      reinterpret_cast<const BrushCoat*>(brush_coat_native_pointer);
  ink::proto::BrushCoat brush_coat_proto;
  EncodeBrushCoat(*brush_coat, brush_coat_proto);
  return SerializeProto(env, brush_coat_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushTip)
(JNIEnv* env, jobject object, jlong brush_tip_native_pointer) {
  const auto* brush_tip =
      reinterpret_cast<const BrushTip*>(brush_tip_native_pointer);
  ink::proto::BrushTip brush_tip_proto;
  EncodeBrushTip(*brush_tip, brush_tip_proto);
  return SerializeProto(env, brush_tip_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushPaint)
(JNIEnv* env, jobject object, jlong brush_paint_native_pointer) {
  const auto* brush_paint =
      reinterpret_cast<const BrushPaint*>(brush_paint_native_pointer);
  ink::proto::BrushPaint brush_paint_proto;
  EncodeBrushPaint(*brush_paint, brush_paint_proto);
  return SerializeProto(env, brush_paint_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushFromProto)
(JNIEnv* env, jobject object, jobject brush_direct_byte_buffer,
 jbyteArray brush_byte_array, jint offset, jint length,
 jboolean throw_on_parse_error) {
  ink::proto::Brush brush_proto;
  if (absl::Status status =
          ParseProtoFromEither(env, brush_direct_byte_buffer, brush_byte_array,
                               offset, length, brush_proto);
      !status.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, status);
    }
    return 0;
  }
  absl::StatusOr<Brush> brush = DecodeBrush(brush_proto);
  if (!brush.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, brush.status());
    }
    return 0;
  }
  return reinterpret_cast<jlong>(new Brush(*std::move(brush)));
}

JNI_METHOD(storage, BrushSerializationNative, jlong,
           newBrushFamilyFromProtoInternal)
(JNIEnv* env, jobject object, jobject brush_family_direct_byte_buffer,
 jbyteArray brush_family_byte_array, jint offset, jint length, jobject callback,
 jboolean throw_on_parse_error) {
  ink::proto::BrushFamily brush_family_proto;
  jclass interfaceClazz = env->GetObjectClass(callback);
  jmethodID on_decode_texture_method =
      env->GetMethodID(interfaceClazz, "onDecodeTexture",
                       "(Ljava/lang/String;[B)Ljava/lang/String;");
  ink::ClientTextureIdProviderAndBitmapReceiver decode_texture_jni_wrapper =
      [env, callback, on_decode_texture_method](
          const std::string& encoded_id,
          absl::Nullable<std::string*> bitmap) -> std::string {
    jstring encoded_id_jstring = env->NewStringUTF(encoded_id.c_str());

    jbyteArray pixel_data_jarray =
        bitmap == nullptr ? nullptr : StdStringToJbyteArray(env, *bitmap);
    // Encoded texture id without bitmap. Call the callback with null bitmap.
    jstring new_id_jstring = static_cast<jstring>(
        env->CallObjectMethod(callback, on_decode_texture_method,
                              encoded_id_jstring, pixel_data_jarray));
    env->DeleteLocalRef(encoded_id_jstring);
    env->DeleteLocalRef(pixel_data_jarray);

    std::string new_id(env->GetStringUTFChars(new_id_jstring, nullptr));
    env->DeleteLocalRef(new_id_jstring);
    return new_id;
  };

  if (absl::Status status = ParseProtoFromEither(
          env, brush_family_direct_byte_buffer, brush_family_byte_array, offset,
          length, brush_family_proto);
      !status.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, status);
    }
    return 0;
  }
  absl::StatusOr<BrushFamily> brush_family =
      DecodeBrushFamily(brush_family_proto, decode_texture_jni_wrapper);
  if (!brush_family.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, brush_family.status());
    }
    return 0;
  }
  return reinterpret_cast<jlong>(new BrushFamily(*std::move(brush_family)));
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushCoatFromProto)
(JNIEnv* env, jobject object, jobject brush_coat_direct_byte_buffer,
 jbyteArray brush_coat_byte_array, jint offset, jint length,
 jboolean throw_on_parse_error) {
  ink::proto::BrushCoat brush_coat_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_coat_direct_byte_buffer, brush_coat_byte_array, offset,
          length, brush_coat_proto);
      !status.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, status);
    }
    return 0;
  }
  absl::StatusOr<BrushCoat> brush_coat = DecodeBrushCoat(brush_coat_proto);
  if (!brush_coat.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, brush_coat.status());
    }
    return 0;
  }
  return reinterpret_cast<jlong>(new BrushCoat(*std::move(brush_coat)));
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushTipFromProto)
(JNIEnv* env, jobject object, jobject brush_tip_direct_byte_buffer,
 jbyteArray brush_tip_byte_array, jint offset, jint length,
 jboolean throw_on_parse_error) {
  ink::proto::BrushTip brush_tip_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_tip_direct_byte_buffer, brush_tip_byte_array, offset,
          length, brush_tip_proto);
      !status.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, status);
    }
    return 0;
  }
  absl::StatusOr<BrushTip> brush_tip = DecodeBrushTip(brush_tip_proto);
  if (!brush_tip.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, brush_tip.status());
    }
    return 0;
  }
  return reinterpret_cast<jlong>(new BrushTip(*std::move(brush_tip)));
}

JNI_METHOD(storage, BrushSerializationNative, jlong, newBrushPaintFromProto)
(JNIEnv* env, jobject object, jobject brush_paint_direct_byte_buffer,
 jbyteArray brush_paint_byte_array, jint offset, jint length,
 jboolean throw_on_parse_error) {
  ink::proto::BrushPaint brush_paint_proto;
  if (absl::Status status = ParseProtoFromEither(
          env, brush_paint_direct_byte_buffer, brush_paint_byte_array, offset,
          length, brush_paint_proto);
      !status.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, status);
    }
    return 0;
  }
  absl::StatusOr<BrushPaint> brush_paint = DecodeBrushPaint(brush_paint_proto);
  if (!brush_paint.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, brush_paint.status());
    }
    return 0;
  }
  return reinterpret_cast<jlong>(new BrushPaint(*std::move(brush_paint)));
}

}  // extern "C"
