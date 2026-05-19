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

#include <jni.h>

#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/status_jni_helper.h"
#include "ink/strokes/internal/jni/mesh_creation_native.h"

using ::ink::jni::ThrowExceptionFromStatusCallback;

extern "C" {

JNI_METHOD(strokes, MeshCreationNative, jlong,
           createClosedShapeFromStrokeInputBatch)
(JNIEnv* env, jobject object, jlong stroke_input_batch_native_pointer) {
  return MeshCreationNative_createClosedShapeFromStrokeInputBatch(
      static_cast<void*>(env), stroke_input_batch_native_pointer,
      &ThrowExceptionFromStatusCallback);
}

}  // extern "C
