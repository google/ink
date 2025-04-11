#ifndef INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_JNI_HELPER_H_
#define INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_JNI_HELPER_H_

#include <jni.h>

#include "ink/brush/brush_family.h"

// Construct a native BrushFamily and return a pointer to it as a long.
jlong CreateBrushFamily(JNIEnv* env, ::ink::BrushFamily::InputModel input_model,
                        jlongArray coat_native_pointer_array,
                        jstring client_brush_family_id);

#endif  // INK_BRUSH_INTERNAL_JNI_BRUSH_FAMILY_JNI_HELPER_H_
