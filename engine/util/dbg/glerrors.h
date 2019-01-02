/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INK_ENGINE_UTIL_DBG_GLERRORS_H_
#define INK_ENGINE_UTIL_DBG_GLERRORS_H_

#include <cstdlib>

#include "ink/engine/util/dbg/glerrors_internal.h"

namespace ink {

// Same as EXPECT/ASSERT, but logs gl specific error info.
// Use GLEXPECT only in one-time initialization.  Any calls during drawing or
// input handling will make us slow especially on WebGL.
#define GLEXPECT(IonGraphics, x) \
  (GLExpectInternal((IonGraphics), (x), __func__, __FILE__, __LINE__))

#if !INK_DEBUG
#define GLASSERT(IonGraphics, x)
#else
#define GLASSERT(IonGraphics, x) \
  (GLExpectInternal((IonGraphics), (x), __func__, __FILE__, __LINE__))
#endif

// Same as EXPECT/ASSERT on the current OpenGL error flag.
#define GLEXPECT_NO_ERROR(IonGraphics) \
  (GLExpectNoErrorInternal((IonGraphics), __func__, __FILE__, __LINE__))

#if !INK_DEBUG
#define GLASSERT_NO_ERROR(IonGraphics)
#else
#define GLASSERT_NO_ERROR(IonGraphics) \
  (GLExpectNoErrorInternal((IonGraphics), __func__, __FILE__, __LINE__))
#endif

}  // namespace ink
#endif  // INK_ENGINE_UTIL_DBG_GLERRORS_H_
