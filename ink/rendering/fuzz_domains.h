// Copyright 2024 Google LLC
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

#ifndef INK_RENDERING_FUZZ_DOMAINS_H_
#define INK_RENDERING_FUZZ_DOMAINS_H_

#include "fuzztest/fuzztest.h"
#include "ink/rendering/bitmap.h"

namespace ink {

// The domain of all valid pixel formats.
fuzztest::Domain<Bitmap::PixelFormat> ArbitraryBitmapPixelFormat();

// The domain of all valid bitmaps up the specified maximum size.
fuzztest::Domain<Bitmap> ValidBitmapWithMaxSize(int max_width, int max_height);

}  // namespace ink

#endif  // INK_RENDERING_FUZZ_DOMAINS_H_
