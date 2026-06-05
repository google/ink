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

#include "absl/base/nullability.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/storage/brush.h"
#include "ink/storage/internal/jni/brush_family_serialization_jni_helper.h"
#include "ink/storage/internal/jni/brush_family_serialization_native.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

using ::ink::ClientTextureIdProviderAndBitmapReceiver;
using ::ink::TextureBitmapProvider;
using ::ink::jni::JniTextureMap;
using ::ink::jni::JvmByteArrayNativeAlloc;
using ::ink::jni::JvmByteArrayNativeAllocCallback;
using ::ink::jni::JvmBytes;
using ::ink::jni::OnDecodeTextureCallback;
using ::ink::jni::OnDecodeTextureNativeCallback;
using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

JNI_METHOD(storage, BrushFamilySerializationNative, jbyteArray, encode)
(JNIEnv* env, jobject object, jlong brush_family_native_pointer,
 absl_nullable jobjectArray texture_map_keys,
 absl_nullable jobjectArray texture_map_values) {
  JvmByteArrayNativeAlloc byte_array_allocator(env);
  JniTextureMap texture_map(env, texture_map_keys, texture_map_values);
  return byte_array_allocator.Release(BrushFamilySerializationNative_encode(
      &byte_array_allocator, brush_family_native_pointer,
      texture_map.TextureIds(), texture_map.Bitmaps(),
      texture_map.BitmapLengths(), texture_map.NumTextures(),
      &JvmByteArrayNativeAllocCallback));
}

JNI_METHOD(storage, BrushFamilySerializationNative, jbyteArray, encodeMultiple)
(JNIEnv* env, jobject object, jlongArray brush_family_native_pointers,
 jobjectArray texture_map_keys, jobjectArray texture_map_values) {
  JvmByteArrayNativeAlloc byte_array_allocator(env);
  JniTextureMap texture_map(env, texture_map_keys, texture_map_values);
  jsize pointers_size = env->GetArrayLength(brush_family_native_pointers);
  jlong* pointers =
      env->GetLongArrayElements(brush_family_native_pointers, nullptr);
  static_assert(sizeof(jlong) == sizeof(int64_t));
  jbyteArray result = byte_array_allocator.Release(
      BrushFamilySerializationNative_encodeMultiple(
          &byte_array_allocator, reinterpret_cast<int64_t*>(pointers),
          pointers_size, texture_map.TextureIds(), texture_map.Bitmaps(),
          texture_map.BitmapLengths(), texture_map.NumTextures(),
          &JvmByteArrayNativeAllocCallback));
  env->ReleaseLongArrayElements(brush_family_native_pointers, pointers,
                                JNI_ABORT);
  return result;
}

JNI_METHOD(storage, BrushFamilySerializationNative, jlong, createFromProto)
(JNIEnv* env, jobject object, jbyteArray brush_family_byte_array, jint length,
 absl_nullable jobject on_decode_texture, jint max_version) {
  JvmBytes brush_family_bytes =
      JvmBytes::FromByteArray(env, brush_family_byte_array);
  OnDecodeTextureCallback decode_texture_callback(env, on_decode_texture);
  return BrushFamilySerializationNative_createFromProto(
      env, &decode_texture_callback, brush_family_bytes.NativeBytes(), length,
      max_version, &ThrowExceptionFromStatusCallback,
      &OnDecodeTextureNativeCallback);
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, jlong, createFromProto)
(JNIEnv* env, jobject object, jbyteArray brush_family_byte_array, jint length,
 absl_nullable jobject on_decode_texture, jint max_version) {
  JvmBytes brush_family_bytes =
      JvmBytes::FromByteArray(env, brush_family_byte_array);
  OnDecodeTextureCallback decode_texture_callback(env, on_decode_texture);
  return MultipleBrushFamiliesNative_createFromProto(
      env, &decode_texture_callback, brush_family_bytes.NativeBytes(), length,
      max_version, &ThrowExceptionFromStatusCallback,
      &OnDecodeTextureNativeCallback);
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, jint, getBrushFamilyCount)
(JNIEnv* env, jobject object, jlong native_pointer) {
  return MultipleBrushFamiliesNative_getBrushFamilyCount(native_pointer);
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, jlong, releaseBrushFamily)
(JNIEnv* env, jobject object, jlong native_pointer, jint index) {
  return MultipleBrushFamiliesNative_releaseBrushFamily(native_pointer, index);
}

JNI_METHOD(storage, MultipleBrushFamiliesNative, void, free)
(JNIEnv* env, jobject object, jlong native_pointer) {
  MultipleBrushFamiliesNative_free(native_pointer);
}

}  // extern "C"
