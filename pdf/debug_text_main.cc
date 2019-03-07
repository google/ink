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

// usage:
//  debug_text --inpdf /path/to/a/random.pdf \
//     [--outpdf /tmp/out.pdf] \
//     [--pngpattern /tmp/out-$0.png]
//
// Renders --inpdf with boxes showing character and line positons. Saves the
// page images to --pngpattern (with $0 replaced by a page number), and saves a
// pdf with boxes to --outpdf.
#include <iostream>
#include <string>
#include <vector>

#include "base/commandlineflags.h"
#include "base/init_google.h"
#include "base/logging.h"
#include "file/base/file.h"
#include "file/base/filesystem.h"
#include "file/base/helpers.h"
#include "file/base/options.h"
#include "third_party/absl/flags/flag.h"
#include "ink/engine/public/types/color.h"
#include "ink/offscreen/pix.h"
#include "ink/pdf/io.h"
#include "ink/pdf/page.h"
#include "ink/pdf/pdf.h"

ABSL_FLAG(string, inpdf, "", "Path of PDF file to read.");
ABSL_FLAG(string, outpdf, "/tmp/out.pdf", "Path of output pdf.");
ABSL_FLAG(string, pngpattern, "/tmp/out-$0.png",
          "Pattern of output png files.");

namespace ink {
void exit() { std::exit(-1); }
}  // namespace ink

using ink::Color;
using ink::Pix;

int main(int argc, char** argv) {
  InitGoogle(argv[0], &argc, &argv, true);

  const string inpdf_path = absl::GetFlag(FLAGS_inpdf);
  const string outpattern = absl::GetFlag(FLAGS_pngpattern);
  const string outpath = absl::GetFlag(FLAGS_outpdf);

  QCHECK(!inpdf_path.empty()) << "Requires --inpdf=<path>";
  QCHECK_OK(file::Exists(inpdf_path, file::Defaults()))
      << inpdf_path << " does not exist.";

  string pdf_data;
  QCHECK_OK(file::GetContents(inpdf_path, &pdf_data, file::Defaults()));
  auto pdf_document = ink::pdf::Document::CreateDocument(pdf_data).ValueOrDie();
  LOG(INFO) << inpdf_path << " has " << pdf_document->PageCount() << " pages";

  Color const start_color = Color::kRed.WithAlpha(.2);
  Color const end_color = Color::kBlue.WithAlpha(.2);

  for (int i = 0; i < pdf_document->PageCount(); i++) {
    auto p = pdf_document->GetPage(i).ValueOrDie();
    auto* tp = p->GetTextPage().ValueOrDie();
    auto const char_count = tp->CharCount();
    for (int i = 0; i < char_count; i++) {
      Color color = Color::Lerp(start_color, end_color,
                                static_cast<float>(i) / char_count);
      QCHECK_OK(p->AddDebugRectangle(
          tp->UnicodeCharacterAt(i).ValueOrDie().GetRect(), color, color,
          ink::pdf::StrokeMode::kNoStroke, ink::pdf::FillMode::kWinding));
    }
    auto const line_count = tp->LineCount();
    for (int i = 0; i < line_count; i++) {
      QCHECK_OK(p->AddDebugRectangle(
          tp->LineAt(i).ValueOrDie().GetRect(), Color::kBlack, Color::kBlack,
          ink::pdf::StrokeMode::kStroke, ink::pdf::FillMode::kNoFill));
    }
    auto dest = p->Render(1).ValueOrDie();
    std::unique_ptr<Pix> pix;
    QCHECK_OK(
        Pix::fromRgba(reinterpret_cast<unsigned char*>(dest->imageByteData()),
                      dest->sizeInPx().width, dest->sizeInPx().height,
                      Pix::NON_PREMULTIPLIED, &pix));
    const string& path = absl::Substitute(outpattern, i);
    LOG(INFO) << "Writing " << path;
    QCHECK_OK(file::SetContents(path, pix->asPng(), file::Overwrite()));
  }
  string out = pdf_document->Write<string>().ValueOrDie();
  LOG(INFO) << "Writing " << outpath;
  QCHECK_OK(file::SetContents(outpath, out, file::Overwrite()));
}
