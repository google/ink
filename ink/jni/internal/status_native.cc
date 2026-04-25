// Copyright 2026 Google LLC
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

#include "ink/jni/internal/status_native.h"

#include "absl/status/status.h"

namespace {

void ThrowExceptionFromStatus(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    absl::Status status) {
  throw_exception_from_status_callback(jni_env_pass_through,
                                       static_cast<int>(status.code()),
                                       status.ToString().c_str());
}

}  // namespace

extern "C" {

int StatusNative_statusCodeOk() {
  return static_cast<int>(absl::StatusCode::kOk);
}

int StatusNative_statusCodeFailedPrecondition() {
  return static_cast<int>(absl::StatusCode::kFailedPrecondition);
}

int StatusNative_statusCodeInvalidArgument() {
  return static_cast<int>(absl::StatusCode::kInvalidArgument);
}

int StatusNative_statusCodeNotFound() {
  return static_cast<int>(absl::StatusCode::kNotFound);
}

int StatusNative_statusCodeOutOfRange() {
  return static_cast<int>(absl::StatusCode::kOutOfRange);
}

int StatusNative_statusCodeUnimplemented() {
  return static_cast<int>(absl::StatusCode::kUnimplemented);
}

void StatusNative_throwExceptionFromOkStatusForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*)) {
  ThrowExceptionFromStatus(jni_env_pass_through,
                           throw_exception_from_status_callback,
                           absl::OkStatus());
}

void StatusNative_throwExceptionFromFailedPreconditionForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message) {
  ThrowExceptionFromStatus(jni_env_pass_through,
                           throw_exception_from_status_callback,
                           absl::FailedPreconditionError(message));
}

void StatusNative_throwExceptionFromInvalidArgumentForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message) {
  ThrowExceptionFromStatus(jni_env_pass_through,
                           throw_exception_from_status_callback,
                           absl::InvalidArgumentError(message));
}

void StatusNative_throwExceptionFromNotFoundForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message) {
  ThrowExceptionFromStatus(jni_env_pass_through,
                           throw_exception_from_status_callback,
                           absl::NotFoundError(message));
}

void StatusNative_throwExceptionFromOutOfRangeForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message) {
  ThrowExceptionFromStatus(jni_env_pass_through,
                           throw_exception_from_status_callback,
                           absl::OutOfRangeError(message));
}

void StatusNative_throwExceptionFromUnimplementedForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message) {
  ThrowExceptionFromStatus(jni_env_pass_through,
                           throw_exception_from_status_callback,
                           absl::UnimplementedError(message));
}

void StatusNative_throwExceptionFromUnknownStatusCodeForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    int status_code, const char* message) {
  ThrowExceptionFromStatus(
      jni_env_pass_through, throw_exception_from_status_callback,
      absl::Status(absl::StatusCode(status_code), message));
}

}  // extern "C"
