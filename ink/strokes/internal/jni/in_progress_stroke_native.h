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

#ifndef INK_STROKES_INTERNAL_JNI_IN_PROGRESS_STROKE_NATIVE_H_
#define INK_STROKES_INTERNAL_JNI_IN_PROGRESS_STROKE_NATIVE_H_

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
} InProgressStrokeNative_Input;

typedef struct {
  float x;
  float y;
} InProgressStrokeNative_Vec;

typedef struct {
  float min_x;
  float min_y;
  float max_x;
  float max_y;
} InProgressStrokeNative_Box;

// Construct a native InProgressStroke and return a pointer to it as a long.
int64_t InProgressStrokeNative_create();

void InProgressStrokeNative_free(int64_t native_pointer);

void InProgressStrokeNative_clear(int64_t native_pointer);

// Starts the stroke with a brush.
void InProgressStrokeNative_start(int64_t native_pointer,
                                  int64_t brush_native_pointer, int noise_seed,
                                  float base_animation_phase);

bool InProgressStrokeNative_enqueueInputs(
    void* jni_env_pass_through, int64_t native_pointer,
    int64_t real_inputs_pointer, int64_t predicted_inputs_pointer,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

bool InProgressStrokeNative_updateShape(
    void* jni_env_pass_through, int64_t native_pointer,
    int64_t j_current_elapsed_time_millis,
    void (*throw_from_status_callback)(void* jni_env, int status_code,
                                       const char* status_str));

void InProgressStrokeNative_finishInput(int64_t native_pointer);

bool InProgressStrokeNative_isInputFinished(int64_t native_pointer);

bool InProgressStrokeNative_isUpdateNeeded(int64_t native_pointer);

bool InProgressStrokeNative_changesWithTime(int64_t native_pointer);

int64_t InProgressStrokeNative_newStrokeFromCopy(int64_t native_pointer);

int64_t InProgressStrokeNative_newStrokeFromPrunedCopy(int64_t native_pointer);

int InProgressStrokeNative_getInputCount(int64_t native_pointer);

int InProgressStrokeNative_getRealInputCount(int64_t native_pointer);

int InProgressStrokeNative_getPredictedInputCount(int64_t native_pointer);

void InProgressStrokeNative_populateInputs(
    int64_t native_pointer, int64_t mutable_stroke_input_batch_pointer,
    int from, int to);

InProgressStrokeNative_Input InProgressStrokeNative_getInput(
    int64_t native_pointer, int index);

float InProgressStrokeNative_getBaseAnimationPhase(int64_t native_pointer);

int InProgressStrokeNative_getBrushCoatCount(int64_t native_pointer);

bool InProgressStrokeNative_hasMeshBounds(int64_t native_pointer,
                                          int coat_index);

// Cannot be called unless InProgressStrokeNative_hasMeshBounds is true for that
// coat index.
InProgressStrokeNative_Box InProgressStrokeNative_getMeshBounds(
    int64_t native_pointer, int coat_index);

bool InProgressStrokeNative_hasUpdatedRegion(int64_t native_pointer);

// Cannot be called unless hasUpdateRegion is true.
InProgressStrokeNative_Box InProgressStrokeNative_getUpdatedRegion(
    int64_t native_pointer);

void InProgressStrokeNative_resetUpdatedRegion(int64_t native_pointer);

int InProgressStrokeNative_getOutlineCount(int64_t native_pointer,
                                           int coat_index);

int InProgressStrokeNative_getOutlineVertexCount(int64_t native_pointer,
                                                 int coat_index,
                                                 int outline_index);

InProgressStrokeNative_Vec InProgressStrokeNative_getOutlinePosition(
    int64_t native_pointer, int coat_index, int outline_index,
    int outline_vertex_index);

InProgressStrokeNative_Vec InProgressStrokeNative_getPosition(
    int64_t native_pointer, int coat_index, int partition_index,
    int vertex_index);

int InProgressStrokeNative_getMeshPartitionCount(int64_t native_pointer,
                                                 int coat_index);

int InProgressStrokeNative_getVertexCount(int64_t native_pointer,
                                          int coat_index, int mesh_index);

// Return a newly allocated copy of the given `Mesh`'s `MeshFormat`.
int64_t InProgressStrokeNative_newCopyOfMeshFormat(int64_t native_pointer,
                                                   int coat_index);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // INK_STROKES_INTERNAL_JNI_IN_PROGRESS_STROKE_NATIVE_H_
