// Copyright 2024 Google LLC
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

#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_JNI_DEFINES_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_JNI_DEFINES_H_

#include "ink/jni/internal/jni_defines.h"

#define BRUSH_FAMILY_CREATE_DEFINITION                                         \
  JNI_METHOD(brush, BrushFamilyNative, jlong, create)                          \
  (JNIEnv * env, jobject object, jlongArray coat_native_pointer_array,         \
   jstring client_brush_family_id) {                                           \
    BrushFamily::InputModel input_model =                                      \
        BrushFamily::InputModel(BrushFamily::SpringModel());                   \
    return ink::CreateBrushFamily(env, input_model, coat_native_pointer_array, \
                                  client_brush_family_id);                     \
  }

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_JNI_DEFINES_H_
