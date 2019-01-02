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

// Ensures that we're building for an ABI that we know something about.

#if !(defined(__linux__) || defined(__ANDROID__) || defined(__asmjs__) || \
      defined(__APPLE__) || defined(__native_client__))
#error \
    "I only know how to be built for Linux, Android" \
    ", asm.js / WebAssembly, iOS, or Native Client (NaCl)"
#endif
