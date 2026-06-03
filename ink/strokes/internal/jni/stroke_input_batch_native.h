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

#ifndef INK_STROKES_INTERNAL_JNI_STROKE_INPUT_BATCH_NATIVE_H_
#define INK_STROKES_INTERNAL_JNI_STROKE_INPUT_BATCH_NATIVE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int tool_type_int;
  float x;
  float y;
  int64_t elapsed_time_millis;
  float stroke_unit_length_cm;
  float pressure;
  float tilt_radians;
  float orientation_radians;
} StrokeInputBatchNative_Input;

int64_t StrokeInputBatchNative_create(void);

void StrokeInputBatchNative_free(int64_t native_pointer);

int StrokeInputBatchNative_getSize(int64_t native_pointer);

StrokeInputBatchNative_Input StrokeInputBatchNative_getStrokeInput(
    int64_t native_pointer, int index);

int64_t StrokeInputBatchNative_getDurationMillis(int64_t native_pointer);

int StrokeInputBatchNative_getToolType(int64_t native_pointer);

float StrokeInputBatchNative_getStrokeUnitLengthCm(int64_t native_pointer);

bool StrokeInputBatchNative_hasStrokeUnitLength(int64_t native_pointer);

bool StrokeInputBatchNative_hasPressure(int64_t native_pointer);

bool StrokeInputBatchNative_hasTilt(int64_t native_pointer);

bool StrokeInputBatchNative_hasOrientation(int64_t native_pointer);

int StrokeInputBatchNative_getNoiseSeed(int64_t native_pointer);

float StrokeInputBatchNative_getBaseAnimationPhase(int64_t native_pointer);

void MutableStrokeInputBatchNative_clear(int64_t native_pointer);

bool MutableStrokeInputBatchNative_appendSingle(
    void* jni_env_pass_through, int64_t native_pointer, int tool_type, float x,
    float y, int64_t elapsed_time_millis, float stroke_unit_length_cm,
    float pressure, float tilt, float orientation,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

bool MutableStrokeInputBatchNative_appendBatch(
    void* jni_env_pass_through, int64_t native_pointer,
    int64_t append_from_native_pointer,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

int64_t MutableStrokeInputBatchNative_newCopy(int64_t native_pointer);

void MutableStrokeInputBatchNative_setNoiseSeed(int64_t native_pointer,
                                                int seed);

void MutableStrokeInputBatchNative_setBaseAnimationPhase(int64_t native_pointer,
                                                         float phase);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_STROKES_INTERNAL_JNI_STROKE_INPUT_BATCH_NATIVE_H_
