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
#include "ink/pdf/io.h"
#include "ink/pdf/pdf.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/proto/export_portable_proto.pb.h"
#include "ink/public/contrib/export.h"

ABSL_FLAG(string, inpdf, "", "Path of PDF file to strip.");
ABSL_FLAG(string, outpdf, "", "Path of PDF file to write or overwrite.");
ABSL_FLAG(bool, overwrite, false,
          "Whether to overwrite outfile, if it exists.");

namespace ink {
void exit() { std::exit(-1); }
}  // namespace ink

int main(int argc, char** argv) {
  InitGoogle(argv[0], &argc, &argv, true);

  const string inpdf_path = absl::GetFlag(FLAGS_inpdf);
  const string outpdf_path = absl::GetFlag(FLAGS_outpdf);
  const bool should_overwrite = absl::GetFlag(FLAGS_overwrite);

  QCHECK(!inpdf_path.empty()) << "Requires --inpdf=<path>";
  QCHECK_OK(file::Exists(inpdf_path, file::Defaults()))
      << inpdf_path << " does not exist.";
  QCHECK(!outpdf_path.empty()) << "Requires --outpdf=<path>";
  if (file::Exists(outpdf_path, file::Defaults()).ok()) {
    QCHECK(should_overwrite)
        << outpdf_path << " exists, but --overwrite was not given.";
  }

  string pdf_data;
  QCHECK_OK(file::GetContents(inpdf_path, &pdf_data, file::Defaults()));
  std::unique_ptr<ink::pdf::Document> pdf_document;
  QCHECK(ink::pdf::Document::CreateDocument(pdf_data, &pdf_document));

  ink::proto::ExportedDocument unused_exported_doc;
  QCHECK(ink::pdf::ReadAndStrip(pdf_document.get(), &unused_exported_doc));

  string out_data;
  QCHECK(pdf_document->Write(&out_data));
  QCHECK_OK(file::SetContents(outpdf_path, out_data, file::Overwrite()));
}
