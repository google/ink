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

#include <jni.h>

#include "ink/brush/brush.h"
#include "ink/brush/brush_coat.h"
#include "ink/brush/brush_family.h"
#include "ink/brush/brush_paint.h"
#include "ink/brush/brush_tip.h"
#include "ink/jni/internal/jni_defines.h"
#include "ink/jni/internal/jni_proto_util.h"
#include "ink/storage/brush.h"
#include "ink/storage/proto/brush.pb.h"

namespace {

using ::ink::Brush;
using ::ink::BrushCoat;
using ::ink::BrushFamily;
using ::ink::BrushPaint;
using ::ink::BrushTip;
using ::ink::jni::SerializeProto;

}  // namespace

extern "C" {

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrush)
(JNIEnv* env, jclass klass, jlong brush_native_pointer) {
  const auto* brush = reinterpret_cast<const Brush*>(brush_native_pointer);
  ink::proto::Brush brush_proto;
  ink::EncodeBrush(*brush, brush_proto);
  return SerializeProto(env, brush_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushFamily)
(JNIEnv* env, jclass klass, jlong brush_family_native_pointer) {
  const auto* brush_family =
      reinterpret_cast<const BrushFamily*>(brush_family_native_pointer);
  ink::proto::BrushFamily brush_family_proto;
  ink::EncodeBrushFamily(*brush_family, brush_family_proto);
  return SerializeProto(env, brush_family_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushCoat)
(JNIEnv* env, jclass klass, jlong brush_coat_native_pointer) {
  const auto* brush_coat =
      reinterpret_cast<const BrushCoat*>(brush_coat_native_pointer);
  ink::proto::BrushCoat brush_coat_proto;
  ink::EncodeBrushCoat(*brush_coat, brush_coat_proto);
  return SerializeProto(env, brush_coat_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushTip)
(JNIEnv* env, jclass klass, jlong brush_tip_native_pointer) {
  const auto* brush_tip =
      reinterpret_cast<const BrushTip*>(brush_tip_native_pointer);
  ink::proto::BrushTip brush_tip_proto;
  ink::EncodeBrushTip(*brush_tip, brush_tip_proto);
  return SerializeProto(env, brush_tip_proto);
}

JNI_METHOD(storage, BrushSerializationNative, jbyteArray, serializeBrushPaint)
(JNIEnv* env, jclass klass, jlong brush_paint_native_pointer) {
  const auto* brush_paint =
      reinterpret_cast<const BrushPaint*>(brush_paint_native_pointer);
  ink::proto::BrushPaint brush_paint_proto;
  ink::EncodeBrushPaint(*brush_paint, brush_paint_proto);
  return SerializeProto(env, brush_paint_proto);
}

}  // extern "C"
