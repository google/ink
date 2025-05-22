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

#ifndef INK_JNI_INTERNAL_JNI_CACHED_METHODS_H_
#define INK_JNI_INTERNAL_JNI_CACHED_METHODS_H_

#include <jni.h>

namespace ink::jni {

void LoadJvmInterface(JNIEnv* env);
void UnloadJvmInterface(JNIEnv* env);

jclass ClassImmutableVec();
jmethodID MethodImmutableVecInitXY();

jclass ClassMutableVec();
jmethodID MethodMutableVecSetX();
jmethodID MethodMutableVecSetY();

jclass ClassImmutableBox();
jmethodID MethodImmutableBoxFromTwoPoints();

jclass ClassMutableBox();
jmethodID MethodMutableBoxSetXBounds();
jmethodID MethodMutableBoxSetYBounds();

jclass ClassBoxAccumulator();
jmethodID MethodBoxAccumulatorReset();
jmethodID MethodBoxAccumulatorPopulateFrom();

jclass ClassImmutableParallelogram();
jmethodID MethodImmutableParallelogramFromCenterDimRotShear();

jclass ClassMutableParallelogram();
jmethodID MethodMutableParallelogramSetCenterDimensionsRotationAndShear();

jclass ClassBrushNative();
jmethodID MethodBrushNativeComposeColorLongFromComponents();

jclass ClassInputToolType();
jmethodID MethodInputToolTypeFrom();

jclass ClassStrokeInput();
jmethodID MethodStrokeInputUpdate();

}  // namespace ink::jni

#endif  // INK_JNI_INTERNAL_JNI_CACHED_METHODS_H_
