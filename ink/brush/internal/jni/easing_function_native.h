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

#ifndef INK_BRUSH_INTERNAL_JNI_EASING_FUNCTION_NATIVE_H_
#define INK_BRUSH_INTERNAL_JNI_EASING_FUNCTION_NATIVE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The native_ptr parameter of these methods contains the raw pointer to a
// C++ EasingFunction object stored in Kotlin on EasingFunction.nativePointer.

// Creates a new heap-allocated copy of the C++ EasingFunction pointed to by
// other_native_ptr and returns a pointer to it as int64_t, suitable for
// wrapping in a Kotlin EasingFunction.
int64_t EasingFunctionNative_createCopyOf(int64_t other_native_ptr);

// Creates a new heap-allocated C++ EasingFunction with
// EasingFunction::Predefined parameters and returns a pointer to it as
// int64_t, suitable for wrapping in a Kotlin EasingFunction. If validation
// fails, calls throw_from_status_callback with the value passed as
// jni_env_pass_through, the error status code as an int, and the error status
// message.
int64_t EasingFunctionNative_createPredefined(
    void* jni_env_pass_through, int value,
    void (*throw_from_status_callback)(void*, int, const char*));

// Creates a new heap-allocated C++ EasingFunction with
// EasingFunction::CubicBezier parameters and returns a pointer to it as
// int64_t, suitable for wrapping in a Kotlin EasingFunction. If validation
// fails, calls throw_from_status_callback as above.
int64_t EasingFunctionNative_createCubicBezier(
    void* jni_env_pass_through, float x1, float y1, float x2, float y2,
    void (*throw_from_status_callback)(void*, int, const char*));

// Creates a new heap-allocated C++ EasingFunction with
// EasingFunction::Linear parameters and returns a pointer to it as
// int64_t, suitable for wrapping in a Kotlin EasingFunction.
// The points argument is a pointer to an array of floats of size
// 2 * num_points, consisting of interleaved x- and y-coordinates. If validation
// fails, calls throw_from_status_callback as above.
int64_t EasingFunctionNative_createLinear(
    void* jni_env_pass_through, const float* points, int num_points,
    void (*throw_from_status_callback)(void*, int, const char*));

// Creates a new heap-allocated C++ EasingFunction with
// EasingFunction::Steps parameters and returns a pointer to it as
// int64_t, suitable for wrapping in a Kotlin EasingFunction. If validation
// fails, calls throw_from_status_callback as above.
int64_t EasingFunctionNative_createSteps(
    void* jni_env_pass_through, int step_count, int step_position,
    void (*throw_from_status_callback)(void*, int, const char*));

// Frees a Kotlin EasingFunction.nativePointer.
void EasingFunctionNative_free(int64_t native_ptr);

int EasingFunctionNative_getParametersType(int64_t native_ptr);

int EasingFunctionNative_getPredefinedValueInt(int64_t native_ptr);

float EasingFunctionNative_getCubicBezierX1(int64_t native_ptr);
float EasingFunctionNative_getCubicBezierY1(int64_t native_ptr);
float EasingFunctionNative_getCubicBezierX2(int64_t native_ptr);
float EasingFunctionNative_getCubicBezierY2(int64_t native_ptr);

int EasingFunctionNative_getLinearNumPoints(int64_t native_ptr);
float EasingFunctionNative_getLinearPointX(int64_t native_ptr, int index);
float EasingFunctionNative_getLinearPointY(int64_t native_ptr, int index);

int EasingFunctionNative_getStepsCount(int64_t native_ptr);
int EasingFunctionNative_getStepsPositionInt(int64_t native_ptr);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_BRUSH_INTERNAL_JNI_EASING_FUNCTION_NATIVE_H_
