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

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/color/color.h"
#include "ink/color/color_space.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/jni/internal/jni_throw_util.h"
#include "ink/rendering/bitmap.h"
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
using ::ink::jni::ParseProtoFromEither;
using ::ink::jni::SerializeProto;
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
 jlong texture_map_native_pointer) {
  const auto* brush_family =
      reinterpret_cast<const BrushFamily*>(brush_family_native_pointer);
  std::map<std::string, ink::VectorBitmap>* map =
      reinterpret_cast<std::map<std::string, ink::VectorBitmap>*>(
          texture_map_native_pointer);

  ink::TextureBitmapProvider texture_bitmap_provider =
      [map](const std::string& texture_id) -> std::optional<ink::VectorBitmap> {
    auto it = map->find(texture_id);
    if (it == map->end()) {
      return std::nullopt;
    }
    return it->second;
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
      env->GetMethodID(interfaceClazz, "onDecodeNativeTexture",
                       "(Ljava/lang/String;II[B)Ljava/lang/String;");
  ink::ClientTextureIdProviderAndBitmapReceiver callback_lambda =
      [env, callback, on_decode_texture_method](
          const std::string& texture_id,
          absl::Nullable<ink::VectorBitmap*> bitmap) -> std::string {
    jstring texture_id_jstring = env->NewStringUTF(texture_id.c_str());

    if (bitmap == nullptr) {
      jbyteArray pixel_data_jarray = env->NewByteArray(0);
      jstring texture_id_out = static_cast<jstring>(
          env->CallObjectMethod(callback, on_decode_texture_method,
                                texture_id_jstring, 0, 0, pixel_data_jarray));
      env->DeleteLocalRef(texture_id_jstring);
      env->DeleteLocalRef(pixel_data_jarray);
      const char* texture_id_chars =
          env->GetStringUTFChars(texture_id_out, nullptr);
      std::string texture_id_string(texture_id_chars);
      env->ReleaseStringUTFChars(texture_id_out, texture_id_chars);
      env->DeleteLocalRef(texture_id_out);
      return texture_id_string;
    } else {
      jbyteArray pixel_data_jarray =
          env->NewByteArray(bitmap->GetPixelData().size());

      env->SetByteArrayRegion(
          pixel_data_jarray, 0, bitmap->GetPixelData().size(),
          reinterpret_cast<const jbyte*>(bitmap->GetPixelData().data()));

      jstring texture_id_out = static_cast<jstring>(env->CallObjectMethod(
          callback, on_decode_texture_method, texture_id_jstring,
          bitmap->width(), bitmap->height(), pixel_data_jarray));

      env->DeleteLocalRef(texture_id_jstring);
      env->DeleteLocalRef(pixel_data_jarray);

      const char* texture_id_chars =
          env->GetStringUTFChars(texture_id_out, nullptr);
      std::string texture_id_string(texture_id_chars);
      env->ReleaseStringUTFChars(texture_id_out, texture_id_chars);
      env->DeleteLocalRef(texture_id_out);
      return texture_id_string;
    }
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
      DecodeBrushFamily(brush_family_proto, callback_lambda);
  if (!brush_family.ok()) {
    if (throw_on_parse_error) {
      ThrowExceptionFromStatus(env, brush_family.status());
    }
    return 0;
  }
  return reinterpret_cast<jlong>(new BrushFamily(*std::move(brush_family)));
}

JNI_METHOD(storage, BrushSerializationNative, jlong, nativeCreateTextureMap)
(JNIEnv* env, jobject object) {
  return reinterpret_cast<jlong>(
      new std::map<std::string, ink::VectorBitmap>());
}

JNI_METHOD(storage, BrushSerializationNative, void,
           nativeAddTextureToTextureMap)
(JNIEnv* env, jobject object, jlong map_native_pointer, jstring texture_id,
 jint width, jint height, jbyteArray pixel_data) {
  std::map<std::string, ink::VectorBitmap>* map =
      reinterpret_cast<std::map<std::string, ink::VectorBitmap>*>(
          map_native_pointer);
  const char* texture_id_chars = env->GetStringUTFChars(texture_id, nullptr);
  std::string texture_id_string(texture_id_chars);
  env->ReleaseStringUTFChars(texture_id, texture_id_chars);
  env->DeleteLocalRef(texture_id);

  jsize pixel_data_size = env->GetArrayLength(pixel_data);
  jbyte* pixel_data_bytes = env->GetByteArrayElements(pixel_data, nullptr);
  std::vector<uint8_t> pixel_data_vector(pixel_data_bytes,
                                         pixel_data_bytes + pixel_data_size);
  ink::VectorBitmap bitmap(width, height, ink::Bitmap::PixelFormat::kRgba8888,
                           ink::Color::Format::kGammaEncoded,
                           ink::ColorSpace::kSrgb, pixel_data_vector);
  env->ReleaseByteArrayElements(pixel_data, pixel_data_bytes, JNI_ABORT);
  map->insert({texture_id_string, bitmap});
}

JNI_METHOD(storage, BrushSerializationNative, void, nativeDestroyTextureMap)
(JNIEnv* env, jobject object, jlong map_native_pointer) {
  delete reinterpret_cast<std::map<std::string, ink::VectorBitmap>*>(
      map_native_pointer);
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
