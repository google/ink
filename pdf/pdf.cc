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

#include "ink/pdf/pdf.h"

#include <string>
#include <vector>

#include "third_party/pdfium/public/fpdfview.h"

namespace ink {
namespace pdf {

// Initialize the pdfium library with the given directory as a font search path.
void InitializePdfium(absl::string_view font_path) {
  std::string path_storage(font_path);
  std::vector<const char*> path_cstrs;
  path_cstrs.push_back(path_storage.c_str());
  path_cstrs.push_back(nullptr);
  FPDF_LIBRARY_CONFIG config;
  config.version = 2;
  config.m_pUserFontPaths = path_cstrs.data();
  config.m_pIsolate = nullptr;
  config.m_v8EmbedderSlot = 0;
  FPDF_InitLibraryWithConfig(&config);
}

void InitializePdfium() { FPDF_InitLibrary(); }

void DestroyPdfium() { FPDF_DestroyLibrary(); }

}  // namespace pdf
}  // namespace ink
