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

#ifndef INK_PUBLIC_CONTRIB_BRUSH_HELPER_H_
#define INK_PUBLIC_CONTRIB_BRUSH_HELPER_H_

#include "ink/engine/public/sengine.h"

namespace ink {
namespace contrib {

void UpdateBrush(SEngine* engine, const ink::proto::BrushType& type,
                 uint32_t rgba, float stroke_width,
                 ink::proto::LineSize::SizeType units,
                 bool use_web_sizes = false);

}  // namespace contrib
}  // namespace ink

#endif  // INK_PUBLIC_CONTRIB_BRUSH_HELPER_H_
