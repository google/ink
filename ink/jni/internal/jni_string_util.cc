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

#include "ink/jni/internal/jni_string_util.h"

#include <jni.h>

namespace ink {
namespace jni {

JStringView::JStringView(JNIEnv* env, jstring j_string) : env_(env) {
  if (j_string != nullptr) {
    j_string_ = static_cast<jstring>(env_->NewGlobalRef(j_string));
    string_view_ =
        absl::string_view(env_->GetStringUTFChars(j_string_, nullptr),
                          env_->GetStringUTFLength(j_string_));
  }
}

JStringView::~JStringView() {
  if (j_string_ != nullptr) {
    env_->ReleaseStringUTFChars(j_string_, string_view_.data());
    env_->DeleteGlobalRef(j_string_);
  }
}

}  // namespace jni
}  // namespace ink
