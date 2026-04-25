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

#ifndef INK_KMP_STATUS_NATIVE_H_
#define INK_KMP_STATUS_NATIVE_H_

#ifdef __cplusplus
extern "C" {
#endif

int StatusNative_statusCodeOk();

int StatusNative_statusCodeFailedPrecondition();

int StatusNative_statusCodeInvalidArgument();

int StatusNative_statusCodeNotFound();

int StatusNative_statusCodeOutOfRange();

int StatusNative_statusCodeUnimplemented();

void StatusNative_throwExceptionFromOkStatusForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*));

void StatusNative_throwExceptionFromFailedPreconditionForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message);

void StatusNative_throwExceptionFromInvalidArgumentForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message);

void StatusNative_throwExceptionFromNotFoundForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message);

void StatusNative_throwExceptionFromOutOfRangeForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message);

void StatusNative_throwExceptionFromUnimplementedForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    const char* message);

void StatusNative_throwExceptionFromUnknownStatusCodeForTesting(
    void* jni_env_pass_through,
    void (*throw_exception_from_status_callback)(void*, int, const char*),
    int status_code, const char* message);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_KMP_STATUS_NATIVE_H_
