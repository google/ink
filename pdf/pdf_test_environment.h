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

#ifndef INK_PDF_PDF_TEST_ENVIRONMENT_H_
#define INK_PDF_PDF_TEST_ENVIRONMENT_H_

#include <memory>
#include <string>

#include "testing/base/public/gunit.h"
#include "third_party/absl/strings/string_view.h"
#include "ink/offscreen/pix.h"
#include "ink/pdf/document.h"
#include "ink/pdf/pdf.h"
#include "ink/proto/document_portable_proto.pb.h"
#include "ink/testing/scuba_test_fixture.h"

namespace ink {
namespace pdf {

extern const char kTestdataPath[];

class PdfTestEnvironment : public testing::ScubaTest, public ::testing::Test {
 public:
  PdfTestEnvironment();
  void SanitizeSnapshotPages(Document* doc, ink::proto::Snapshot* snapshot);
  std::unique_ptr<Document> LoadPDF(absl::string_view path);
  ink::proto::Snapshot LoadSnapshot(absl::string_view path);
  std::unique_ptr<ink::Pix> RenderPage(const Page& page,
                                       const Rect& crop = {0, 0, 0, 0});

  ::string Serialize(const pdf::Document& doc);

  void CompareWithPdfGoldens(absl::string_view prefix,
                             absl::string_view actual_pdf_contents,
                             absl::string_view golden_pdf_path);
  void CompareWithPdfGoldens(absl::string_view actual_pdf_contents,
                             absl::string_view golden_pdf_path);

  ::string TestdataImpl(
      std::initializer_list<absl::string_view> filename_pieces);

  ::string AsPng(const ClientBitmap& bitmap) const;

  // Return the absolute path of the given testdata-relative resource.
  template <typename... S>
  ::string testdata(const S&... parts) {
    return TestdataImpl({FLAGS_test_srcdir, kTestdataPath, parts...});
  }

 protected:
  // Permit use as a standalone environment in a benchmark.
  void TestBody() override {}
};

}  // namespace pdf
}  // namespace ink

#endif  // INK_PDF_PDF_TEST_ENVIRONMENT_H_
