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

#include <memory>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "ink/brush/brush_family.h"
#include "ink/storage/brush.h"
#include "ink/storage/proto/brush.pb.h"
#include "ink/storage/proto/brush_family.pb.h"

namespace ink::jni {

using MultipleBrushFamilies = std::vector<std::unique_ptr<BrushFamily>>;

inline jlong NewNativeMultipleBrushFamilies(MultipleBrushFamilies&& families) {
  return reinterpret_cast<jlong>(
      new MultipleBrushFamilies(std::forward<MultipleBrushFamilies>(families)));
}

inline MultipleBrushFamilies& CastToMutableMultipleBrushFamilies(
    jlong native_pointer) {
  return *reinterpret_cast<MultipleBrushFamilies*>(native_pointer);
}

inline void DeleteMultipleBrushFamilies(jlong native_pointer) {
  delete reinterpret_cast<MultipleBrushFamilies*>(native_pointer);
}

TextureBitmapProvider CreateTextureBitmapProvider(
    JNIEnv* env, jobjectArray texture_map_keys,
    jobjectArray texture_map_values);

ClientTextureIdProviderAndBitmapReceiver CreateDecodeTextureJniWrapper(
    JNIEnv* env, absl_nullable jobject callback);

}  // namespace ink::jni

#endif  // INK_STORAGE_INTERNAL_JNI_BRUSH_FAMILY_SERIALIZATION_JNI_HELPER_H_
