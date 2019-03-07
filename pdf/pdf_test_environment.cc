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

#include "ink/pdf/pdf_test_environment.h"

#include "base/commandlineflags.h"
#include "file/base/file.h"
#include "file/base/helpers.h"
#include "file/base/options.h"
#include "file/base/path.h"
#include "image/base/rawimage.h"
#include "testing/base/public/gmock.h"
#include "testing/lib/sponge/undeclared_outputs.h"
#include "third_party/absl/flags/flag.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/str_join.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/pdf/io.h"
#include "util/regexp/re2/re2.h"

DEFINE_string(
    generate_golden_to, "",
    "If empty, the golden test requires that the generated output "
    "exactly match the golden files. If non-empty, the golden files "
    "in the given directory are overwritten with the generated output.");

using absl::StrCat;
using absl::StrJoin;
using ink::Pix;
using testing::sponge::SaveUndeclaredOutput;

namespace ink {
namespace pdf {

const char kTestdataPath[] = "/google3/ink/pdf/testdata/";

namespace {
REGISTER_MODULE_INITIALIZER(initialize_pdfium, {
  InitializePdfium(
      StrCat(FLAGS_test_srcdir, "google3/ink/pdf/fonts"));
});

// Replace the randomly-generated document IDs in the given serialized PDF
// with strings of 0s of the same length. This gives you unchanging golden
// file generation.
void StripTrailerIds(::string* pdf_bytes) {
  ::string id1;
  ::string id2;
  QCHECK(RE2::PartialMatch(*pdf_bytes, "/ID\\[<([0-9A-F]+)><([0-9A-F]+)>", &id1,
                           &id2));
  const auto replacement = StrCat("/ID[<", std::string(id1.size(), '0'), "><",
                                  std::string(id2.size(), '0'), ">");
  QCHECK(
      RE2::Replace(pdf_bytes, "/ID\\[<([0-9A-F]+)><([0-9A-F]+)>", replacement));
}

std::unique_ptr<Pix> PixFromClientBitmap(const ClientBitmap& bitmap) {
  std::unique_ptr<Pix> pix;
  if (bitmap.bytesPerTexel() == 4) {
    QCHECK_OK(Pix::fromRgba(
        reinterpret_cast<unsigned const char*>(bitmap.imageByteData()),
        bitmap.sizeInPx().width, bitmap.sizeInPx().height,
        Pix::NON_PREMULTIPLIED, &pix));
  } else {
    QCHECK_OK(Pix::fromRgb(
        reinterpret_cast<unsigned const char*>(bitmap.imageByteData()),
        bitmap.sizeInPx().width, bitmap.sizeInPx().height, &pix));
  }
  return pix;
}

}  // namespace

PdfTestEnvironment::PdfTestEnvironment()
    : testing::ScubaTest("/google3/ink/pdf/scuba_goldens") {
}

void PdfTestEnvironment::SanitizeSnapshotPages(Document* doc,
                                               ink::proto::Snapshot* snapshot) {
  for (int i = 0; i < doc->PageCount(); ++i) {
    auto* page = snapshot->add_per_page_properties();
    Rect dim = doc->GetPageBounds(i).ValueOrDie();
    page->set_uuid(StrCat("page", i));
    page->set_width(dim.Dim()[0]);
    page->set_height(dim.Dim()[1]);
  }
  // Set all elements in the snapshot to page 0.
  for (auto& elem : *snapshot->mutable_element()) {
    elem.set_group_uuid("page0");
  }
}

::string PdfTestEnvironment::Serialize(const pdf::Document& doc) {
  auto s = doc.Write<::string>().ValueOrDie();
  StripTrailerIds(&s);
  return s;
}

::string PdfTestEnvironment::TestdataImpl(
    std::initializer_list<absl::string_view> filename_pieces) {
  return StrJoin(filename_pieces, "");
}

std::unique_ptr<Document> PdfTestEnvironment::LoadPDF(absl::string_view path) {
  string pdf_data;
  QCHECK_OK(file::GetContents(path, &pdf_data, file::Defaults()));
  return Document::CreateDocument(pdf_data).ValueOrDie();
}

ink::proto::Snapshot PdfTestEnvironment::LoadSnapshot(absl::string_view path) {
  string snapshot_data;
  QCHECK_OK(file::GetContents(path, &snapshot_data, file::Defaults()));
  ink::proto::Snapshot snapshot;
  QCHECK(snapshot.ParseFromString(snapshot_data))
      << "Could not parse " << path << " as snapshot.";
  return snapshot;
}

std::unique_ptr<Pix> PdfTestEnvironment::RenderPage(const Page& page,
                                                    const Rect& crop) {
  // Render to a standard max dimension.
  static constexpr int kMaxDimension = 600;
  Rect bounds = page.Bounds();
  QCHECK_GT(bounds.Area(), 0) << "0 page size";
  const float scale = static_cast<float>(kMaxDimension) /
                      std::max(bounds.Width(), bounds.Height());
  std::unique_ptr<Pix> pix =
      PixFromClientBitmap(*page.Render(scale).ValueOrDie());
  if (!crop.Empty()) {
    QCHECK_OK(pix->Crop(crop.Left(), crop.Bottom(), crop.Right(), crop.Top()));
  }
  return pix;
}

void PdfTestEnvironment::CompareWithPdfGoldens(
    absl::string_view actual_pdf_contents, absl::string_view golden_pdf_path) {
  CompareWithPdfGoldens(GetDefaultKey(), actual_pdf_contents, golden_pdf_path);
}

void PdfTestEnvironment::CompareWithPdfGoldens(
    absl::string_view prefix, absl::string_view actual_pdf_contents,
    absl::string_view golden_pdf_path) {
  auto doc = Document::CreateDocument(actual_pdf_contents).ValueOrDie();
  int page_count = doc->PageCount();

  string generate_golden_to = absl::GetFlag(FLAGS_generate_golden_to);
  if (!generate_golden_to.empty()) {
    QCHECK_OK(file::SetContents(
        file::JoinPath(generate_golden_to, file::Basename(golden_pdf_path)),
        actual_pdf_contents, file::Overwrite()));
    return;
  }

  // Save PDFs as undeclared outputs for human inspection.
  string expected_pdf_contents;
  QCHECK_OK(file::GetContents(golden_pdf_path, &expected_pdf_contents,
                              file::Defaults()))
      << "could not read expected data from " << golden_pdf_path
      << ". Perhaps you need to --generate_golden_to?";
  SaveUndeclaredOutput(StrCat(prefix, "-expected.pdf"),
                       StrCat(file::Stem(golden_pdf_path), " (expected)"),
                       "application/pdf", expected_pdf_contents);
  SaveUndeclaredOutput(
      StrCat(prefix, "-actual.pdf"),
      StrCat(prefix, "-", file::Stem(golden_pdf_path), " (actual)"),
      "application/pdf", actual_pdf_contents);

  for (int i = 0; i < page_count; i++) {
    auto actual_page = doc->GetPage(i).ValueOrDie();
    auto pix = RenderPage(*actual_page);
    CompareWithGolden(StrCat(prefix, "-p", i), pix->asPng());
  }
}

::string PdfTestEnvironment::AsPng(const ClientBitmap& bitmap) const {
  return PixFromClientBitmap(bitmap)->asPng();
}

}  // namespace pdf
}  // namespace ink
