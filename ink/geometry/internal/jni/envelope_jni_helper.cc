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

#include "ink/geometry/internal/jni/envelope_jni_helper.h"

#include <jni.h>

#include "absl/log/check.h"
#include "ink/geometry/envelope.h"
#include "ink/geometry/rect.h"
#include "ink/jni/internal/jni_defines.h"

namespace ink {

void FillJMutableEnvelope(JNIEnv* env, const Envelope& envelope,
                          jobject mutable_envelope) {
  jclass mutable_envelope_class = env->GetObjectClass(mutable_envelope);
  if (envelope.IsEmpty()) {
    jmethodID reset_method =
        env->GetMethodID(mutable_envelope_class, "reset",
                         "()L" INK_PACKAGE "/geometry/BoxAccumulator;");
    CHECK(reset_method);
    env->CallObjectMethod(mutable_envelope, reset_method);
  } else {
    const Rect rect = *envelope.AsRect();
    jmethodID overwrite_method =
        env->GetMethodID(mutable_envelope_class, "overwriteFrom",
                         "(FFFF)L" INK_PACKAGE "/geometry/BoxAccumulator;");
    CHECK(overwrite_method);
    env->CallObjectMethod(mutable_envelope, overwrite_method, rect.XMin(),
                          rect.YMin(), rect.XMax(), rect.YMax());
  }
}

}  // namespace ink
