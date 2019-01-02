// Copyright 2018 Google LLC
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

#include "ink/pdf/text_object.h"

#include "ink/pdf/internal.h"

namespace ink {
namespace pdf {

using internal::Clamp;

void Text::SetColor(int red, int green, int blue, int alpha) {
  FPDFText_SetFillColor(WrappedObject(), Clamp(red, 0, 255),
                        Clamp(green, 0, 255), Clamp(blue, 0, 255),
                        Clamp(alpha, 0, 255));
}

}  // namespace pdf
}  // namespace ink
