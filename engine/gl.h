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

#ifndef INK_ENGINE_GL_H_
#define INK_ENGINE_GL_H_

#include "geo/render/ion/portgfx/glheaders.h"
#include "geo/render/ion/external/tess/glutess.h"

// screwed up by Xlib, which is included by glheaders.h.
#undef Status
#undef Bool
#undef True
#undef False
#undef None
// --

#endif  // INK_ENGINE_GL_H_
