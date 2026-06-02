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

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/storage/brush.h"
#include "ink/storage/internal/jni/brush_family_serialization_jni_helper.h"
#include "ink/storage/internal/jni/brush_family_serialization_native_helper.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

using ::ink::BrushFamily;
using ::ink::ClientTextureIdProviderAndBitmapReceiver;
using ::ink::DecodeBrushFamily;
using ::ink::DecodeMultipleBrushFamilies;
using ::ink::EncodeBrushFamily;
using ::ink::EncodeMultipleBrushFamilies;
using ::ink::TextureBitmapProvider;
using ::ink::jni::CreateTextureBitmapProvider;
using ::ink::jni::OnDecodeTextureCallback;
using ::ink::jni::OnDecodeTextureJniWrapper;
using ::ink::jni::ParseProtoFromByteArray;
using ::ink::jni::SerializeProto;
using ::ink::jni::ThrowExceptionFromStatus;
using ::ink::native::CastToBrushFamily;
using ::ink::native::CastToMutableMultipleBrushFamilies;
using ::ink::native::DeleteMultipleBrushFamilies;
using ::ink::native::IntToVersion;
using ::ink::native::NewNativeBrushFamily;
using ::ink::native::NewNativeMultipleBrushFamilies;

extern "C" {

JNI_METHOD(storage, BrushFamilySerializationNative, jbyteArray, encode)
(JNIEnv* env, jobject object, jlong brush_family_native_pointer,
 jobjectArray texture_map_keys, jobjectArray texture_map_values) {
  TextureBitmapProvider texture_bitmap_provider =
      CreateTextureBitmapProvider(env, texture_map_keys, texture_map_values);

  ink::proto::BrushFamily brush_family_proto;
  EncodeBrushFamily(CastToBrushFamily(brush_family_native_pointer),
                    brush_family_proto, texture_bitmap_provider);
  return SerializeProto(env, brush_family_proto);
}

JNI_METHOD(storage, BrushFamilySerializationNative, jbyteArray, encodeMultiple)
(JNIEnv* env, jobject object, jlongArray brush_family_native_pointers,
 jobjectArray texture_map_keys, jobjectArray texture_map_values) {
  TextureBitmapProvider texture_bitmap_provider =
      CreateTextureBitmapProvider(env, texture_map_keys, texture_map_values);

  jsize pointers_length = env->GetArrayLength(brush_family_native_pointers);
  jlong* pointers =
      env->GetLongArrayElements(brush_family_native_pointers, nullptr);
  // Pass by reference to avoid a copy of all the brush families.
  std::vector<std::reference_wrapper<const BrushFamily>> families;
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

JNI_METHOD(storage, BrushFamilySerializationNative, jlong, createFromProto)
(JNIEnv* env, jobject object, jbyteArray brush_family_byte_array, jint length,
 absl_nullable jobject callback, jint max_version) {
  ink::proto::BrushFamily brush_family_proto;
  constexpr int kOffset = 0;
  if (absl::Status status = ParseProtoFromByteArray(
          env, brush_family_byte_array, kOffset, length, brush_family_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }

  OnDecodeTextureCallback on_decode_texture_callback(env, callback);
  absl::StatusOr<BrushFamily> brush_family = DecodeBrushFamily(
      brush_family_proto, OnDecodeTextureJniWrapper(on_decode_texture_callback),
      IntToVersion(max_version));
  if (!brush_family.ok()) {
    ThrowExceptionFromStatus(env, brush_family.status());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(brush_family));
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, jlong, createFromProto)
(JNIEnv* env, jobject object, jbyteArray brush_family_byte_array, jint length,
 absl_nullable jobject callback, jint max_version) {
  constexpr int kOffset = 0;
  ink::proto::BrushFamily brush_family_proto;
  if (absl::Status status = ParseProtoFromByteArray(
          env, brush_family_byte_array, kOffset, length, brush_family_proto);
      !status.ok()) {
    ThrowExceptionFromStatus(env, status);
    return 0;
  }

  OnDecodeTextureCallback on_decode_texture_callback(env, callback);
  absl::StatusOr<std::vector<BrushFamily>> brush_families =
      DecodeMultipleBrushFamilies(
          brush_family_proto,
          OnDecodeTextureJniWrapper(on_decode_texture_callback),
          IntToVersion(max_version));
  if (!brush_families.ok()) {
    ThrowExceptionFromStatus(env, brush_families.status());
    return 0;
  }

  std::vector<std::unique_ptr<BrushFamily>> result;
  result.reserve(brush_families->size());
  for (BrushFamily& brush_family : *brush_families) {
    result.push_back(std::make_unique<BrushFamily>(std::move(brush_family)));
  }
  return NewNativeMultipleBrushFamilies(std::move(result));
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, jint, getBrushFamilyCount)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return CastToMutableMultipleBrushFamilies(native_pointer).size();
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, jlong, releaseBrushFamily)
(JNIEnv* env, jobject object, jlong native_pointer, jint index) {
  // This is already a heap-allocated BrushFamily, handed off so that the new
  // Kotlin BrushFamily will start managing cleanup.
  return reinterpret_cast<jlong>(
      CastToMutableMultipleBrushFamilies(native_pointer)[index].release());
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  DeleteMultipleBrushFamilies(native_pointer);
}

}  // extern "C"
