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

#ifndef INK_PDF_TEXT_PAGE_H_
#define INK_PDF_TEXT_PAGE_H_

#include <memory>
#include <string>

#include "testing/production_stub/public/gunit_prod.h"
#include "third_party/absl/base/attributes.h"
#include "third_party/pdfium/public/cpp/fpdf_scopers.h"
#include "third_party/pdfium/public/fpdf_text.h"
#include "third_party/pdfium/public/fpdfview.h"
#include "ink/engine/geometry/primitives/rect.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/public/types/status_or.h"

namespace ink {
namespace pdf {

// Line height is multiplied by this number to expand the horizontal region in
// which a user could select a line or character and still be considered in the
// line/character.
extern const float kLineHitMarginFactor;

bool OverlapsOnYAxis(Rect a, Rect b);

class Candidate {
 public:
  enum Direction { L, R };
  Candidate(int line_index, int char_index, Direction direction)
      : line_index_(line_index),
        char_index_(char_index),
        direction_(direction) {}

  std::string ToString() const {
    return Substitute("Candidate($0$1:$2$3)",
                      direction_ == Direction::L ? "]" : "", line_index_,
                      char_index_, direction_ == Direction::R ? "[" : "");
  }

  bool operator==(const Candidate& rhs) const {
    return std::tie(line_index_, char_index_, direction_) ==
           std::tie(rhs.line_index_, rhs.char_index_, rhs.direction_);
  }

  int line_index_;
  int char_index_;
  Direction direction_;
};

class UnicodeCharacter {
 public:
  UnicodeCharacter(int code_point, Rect rect)
      : code_point_(code_point), rect_(rect) {}
  UnicodeCharacter() : UnicodeCharacter(0, Rect()) {}

  Rect GetRect() const;
  unsigned int GetCodePoint() const;
  bool IsEOL() const;

  float Left() const;
  float Right() const;

  void ExpandVerticallyToFit(float top, float bottom);

  // Expands the width of a UnicodeCharacter Rect right or left by given
  // distance. Negative distance expands left. Positive distance expands right.
  void ExpandRectWidth(float distance);

 private:
  unsigned int code_point_;
  Rect rect_;
};

class Line {
 public:
  Line() : rect_() {}
  Rect GetRect() const;
  float Top() const;
  float Bottom() const;
  float Left() const;
  float Right() const;

  std::string ToString() const;

 private:
  void AddChar(UnicodeCharacter uc);

  std::vector<UnicodeCharacter> unichars_;
  Rect rect_;

  friend class TextPage;

  FRIEND_TEST(TextPageTest, TestExpandLineRect);
  FRIEND_TEST(TextPageTest, TestAddChar);
};

class TextPage {
 public:
  explicit TextPage(FPDF_TEXTPAGE text_page);

  int CharCount() const;
  StatusOr<UnicodeCharacter> UnicodeCharacterAt(int index) const;
  StatusOr<Line> LineAt(int index) const;
  int LineCount() const;

  // Given the start and end points of a gesture, infers a selection of lines
  // between these points and populates the given vector with the wanted
  // selection rectangles in coordinates transformed by the given matrix.
  void GetSelectionRects(glm::vec2 p, glm::vec2 q, glm::mat4 page_to_world,
                         std::vector<Rect>* out);

  // Given vec2 from, this function finds the selection between from and the end
  // of the page, and populates the given vector with the wanted selection
  // rectangles in coordinates transformed by the given matrix.
  void GetSelectionRectsToEnd(glm::vec2 from, glm::mat4 page_to_world,
                              std::vector<Rect>* out);

  // Given vec2 to, this function finds the selection between the beginning of
  // the page and to, and populates the given vector with the wanted selection
  // rectangles in coordinates transformed by the given matrix.
  void GetSelectionRectsFromBeginning(glm::vec2 to, glm::mat4 page_to_world,
                                      std::vector<Rect>* out);

  // Populates the given vector with the all of this page's selection
  // rectangles, in coordinates transformed by the given matrix.
  void GetSelectionRects(glm::mat4 page_to_world, std::vector<Rect>* out);

  // Given the start and end points of a gesture, infers a selection of lines
  // between these points and returns a string containing text of the selection.
  std::string GetSelectionText(glm::vec2 p, glm::vec2 q) const;

  // Given vec2 from, returns text between from and the end of the page.
  std::string GetSelectionTextToEnd(glm::vec2 from) const;

  // Given vec2 to, returns text between the beginning of the page and to.
  std::string GetSelectionTextFromBeginning(glm::vec2 to) const;

  // Returns all text on the page.
  std::string GetText() const;

  // Return true if the given point is "in text", i.e., is close enough to text
  // to be considered part of a text selection.
  bool IsInText(glm::vec2 p) const;

  // Infers 0-2 possible meanings (candidates) for characters in
  // a line that a gesture may refer to. 0, if the point is too far from any
  // text; 1, if it is after some text but not before some other text, or vice
  // versa; 2, if the point is between two possible intended targets.
  // This is made public for interactive debugging.
  std::vector<Candidate> CandidatesAt(glm::vec2 point) const;

  // Returns the rectangle ("hit box") for some selection Candidate.
  // Visible for interactive debugging.
  Rect CandidateRect(const Candidate& c) const;

  friend bool operator<(Candidate const& a, Candidate const& b);

  // TextPage is neither copyable nor movable.
  TextPage(const TextPage&) = delete;
  TextPage& operator=(const TextPage&) = delete;

 private:
  ABSL_MUST_USE_RESULT Status GenerateIndex();

  // Adjusts height and width of character to be uniform within the line.
  void ExpandCharactersToFillLine(Line* line);

  StatusOr<Rect> CharRectAt(int index) const;
  int CodePointAt(int index) const;

  // Chooses a start and end candidate to define the bounding points of a
  // selection.
  void SortCandidatesInPlace(std::vector<Candidate>* ca,
                             std::vector<Candidate>* cb) const;

  // Adds part of a line that is inferred to be between two candidates.
  void AppendLineChunk(const Candidate& a, const Candidate& b,

                       std::vector<Rect>* selection,
                       glm::mat4 transform = glm::mat4(1));

  std::string GetSelectionTextImpl(glm::vec2 a, glm::vec2 b) const;

  ScopedFPDFTextPage text_page_;
  std::vector<Line> lines_;

  friend class Page;

  // Allows TextPageTests to test private TextPage functions and access lines_
  // variable.
  FRIEND_TEST(TextPageTest, TestSortCandidatesInPlace);
  FRIEND_TEST(TextPageTest, TestAppendLineChunk);
  FRIEND_TEST(TextPageTest, TestGetCandidates);
  FRIEND_TEST(TextPageTest, TestGetCandidatesColumns);
  FRIEND_TEST(TextPageTest, TestGetSelectionRects);
  FRIEND_TEST(TextPageTest, TestGetSelectionText);
  FRIEND_TEST(TextPageTest, TestGetSelectionTextToEnd);
  FRIEND_TEST(TextPageTest, TestGetSelectionTextFromBeginning);
  FRIEND_TEST(TextPageTest, TestGetText);
  FRIEND_TEST(TextPageTest, TestPointBetweenTwoCharacters);
  FRIEND_TEST(TextPageTest, TestPointOffEndOfLine);
};

}  // namespace pdf
}  // namespace ink

#endif  // INK_PDF_TEXT_PAGE_H_
