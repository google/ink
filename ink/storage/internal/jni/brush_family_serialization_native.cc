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

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/internal/jni/brush_native_helper.h"
#include "ink/storage/brush.h"
#include "ink/storage/internal/jni/brush_family_serialization_native_helper.h"
#include "ink/storage/internal/jni/byte_array_native_helper.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

using ::ink::BrushFamily;
using ::ink::ClientTextureIdProviderAndBitmapReceiver;
using ::ink::DecodeBrushFamily;
using ::ink::DecodeMultipleBrushFamilies;
using ::ink::EncodeBrushFamily;
using ::ink::EncodeMultipleBrushFamilies;
using ::ink::TextureBitmapProvider;
using ::ink::native::CastToBrushFamily;
using ::ink::native::CastToMutableMultipleBrushFamilies;
using ::ink::native::ClientTextureIdProviderAndBitmapReceiverFromNativeCallback;
using ::ink::native::DeleteMultipleBrushFamilies;
using ::ink::native::IntToVersion;
using ::ink::native::NewNativeBrushFamily;
using ::ink::native::NewNativeByteArrayFromProto;
using ::ink::native::NewNativeMultipleBrushFamilies;
using ::ink::native::TextureBitmapProviderFromNativeArrays;

namespace {

bool ParseBrushFamilyProtoFromByteArray(
    void* jni_env_pass_through, const int8_t* byte_array, int size,
    void (*throw_from_status_callback)(void*, int, const char*),
    ink::proto::BrushFamily& brush_family_proto) {
  ABSL_CHECK(byte_array != nullptr);
  bool success = brush_family_proto.ParseFromArray(byte_array, size);
  if (!success) {
    throw_from_status_callback(
        jni_env_pass_through,
        static_cast<int>(absl::StatusCode::kInvalidArgument),
        "Failed to parse ink.proto.BrushFamily.");
    return false;
  }
  return true;
}

}  // namespace

extern "C" {

int64_t BrushFamilySerializationNative_createFromProto(
    void* jni_env_pass_through, void* on_decode_texture_pass_through,
    const int8_t* byte_array, int size, int max_version,
    void (*throw_from_status_callback)(void*, int, const char*),
    const char* (*on_decode_texture_callback)(void* pass_through,
                                              const char* encoded_id,
                                              const int8_t* bitmap,
                                              int bitmap_size)) {
  ink::proto::BrushFamily brush_family_proto;
  if (!ParseBrushFamilyProtoFromByteArray(jni_env_pass_through, byte_array,
                                          size, throw_from_status_callback,
                                          brush_family_proto)) {
    return 0;
  }
  absl::StatusOr<BrushFamily> brush_family = DecodeBrushFamily(
      brush_family_proto,
      ClientTextureIdProviderAndBitmapReceiverFromNativeCallback(
          on_decode_texture_pass_through, on_decode_texture_callback),
      IntToVersion(max_version));
  if (!brush_family.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(brush_family.status().code()),
                               brush_family.status().ToString().c_str());
    return 0;
  }
  return NewNativeBrushFamily(*std::move(brush_family));
}

int8_t* BrushFamilySerializationNative_encode(
    void* alloc_native_array_pass_through, int64_t native_pointer,
    const char** texture_ids, const int8_t** bitmaps, const int* bitmap_lengths,
    int num_textures,
    int8_t* (*alloc_native_array_callback)(void* pass_through, int size)) {
  TextureBitmapProvider texture_bitmap_provider =
      TextureBitmapProviderFromNativeArrays(texture_ids, bitmaps,
                                            bitmap_lengths, num_textures);
  ink::proto::BrushFamily brush_family_proto;
  EncodeBrushFamily(CastToBrushFamily(native_pointer), brush_family_proto,
                    texture_bitmap_provider);
  return NewNativeByteArrayFromProto(alloc_native_array_pass_through,
                                     brush_family_proto,
                                     alloc_native_array_callback);
}

int8_t* BrushFamilySerializationNative_encodeMultiple(
    void* alloc_native_array_pass_through, int64_t* native_pointers,
    int num_brush_families, const char** texture_ids, const int8_t** bitmaps,
    const int* bitmap_lengths, int num_textures,
    int8_t* (*alloc_native_array_callback)(void* pass_through, int size)) {
  TextureBitmapProvider texture_bitmap_provider =
      TextureBitmapProviderFromNativeArrays(texture_ids, bitmaps,
                                            bitmap_lengths, num_textures);
  std::vector<std::reference_wrapper<const BrushFamily>> brush_families;
  brush_families.reserve(num_brush_families);
  for (int i = 0; i < num_brush_families; ++i) {
    brush_families.push_back(CastToBrushFamily(native_pointers[i]));
  }
  ink::proto::BrushFamily brush_family_proto;
  EncodeMultipleBrushFamilies(brush_families, brush_family_proto,
                              texture_bitmap_provider);
  return NewNativeByteArrayFromProto(alloc_native_array_pass_through,
                                     brush_family_proto,
                                     alloc_native_array_callback);
}

int64_t MultipleBrushFamiliesNative_createFromProto(
    void* jni_env_pass_through, void* on_decode_texture_pass_through,
    const int8_t* byte_array, int size, int max_version,
    void (*throw_from_status_callback)(void*, int, const char*),
    const char* (*on_decode_texture_callback)(void* pass_through,
                                              const char* encoded_id,
                                              const int8_t* bitmap,
                                              int bitmap_size)) {
  ink::proto::BrushFamily brush_family_proto;
  if (!ParseBrushFamilyProtoFromByteArray(jni_env_pass_through, byte_array,
                                          size, throw_from_status_callback,
                                          brush_family_proto)) {
    return 0;
  }
  absl::StatusOr<std::vector<BrushFamily>> brush_families =
      DecodeMultipleBrushFamilies(
          brush_family_proto,
          ClientTextureIdProviderAndBitmapReceiverFromNativeCallback(
              on_decode_texture_pass_through, on_decode_texture_callback),
          IntToVersion(max_version));
  if (!brush_families.ok()) {
    throw_from_status_callback(jni_env_pass_through,
                               static_cast<int>(brush_families.status().code()),
                               brush_families.status().ToString().c_str());
    return 0;
  }
  std::vector<std::unique_ptr<BrushFamily>> result;
  result.reserve(brush_families->size());
  for (BrushFamily& brush_family : *brush_families) {
    result.push_back(std::make_unique<BrushFamily>(std::move(brush_family)));
  }
  return NewNativeMultipleBrushFamilies(std::move(result));
}

int MultipleBrushFamiliesNative_getBrushFamilyCount(int64_t native_pointer) {
  return CastToMutableMultipleBrushFamilies(native_pointer).size();
}

int64_t MultipleBrushFamiliesNative_releaseBrushFamily(int64_t native_pointer,
                                                       int index) {
  // This is already a heap-allocated BrushFamily, handed off so that the new
  // Kotlin BrushFamily will start managing cleanup.
  return reinterpret_cast<int64_t>(
      CastToMutableMultipleBrushFamilies(native_pointer)[index].release());
}

void MultipleBrushFamiliesNative_free(int64_t native_pointer) {
  DeleteMultipleBrushFamilies(native_pointer);
}

}  // extern "C"
