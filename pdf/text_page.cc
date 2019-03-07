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

#include "ink/pdf/text_page.h"

#include "third_party/absl/base/macros.h"
#include "ink/engine/geometry/algorithms/transform.h"
#include "ink/pdf/internal.h"

namespace ink {
namespace pdf {

// hyphen code point used to determine if a char is a hyphenated line break
const unsigned int kLineSplittingHyphen = 0x02;

const float kLineHitMarginFactor = .5;

// The acceptable fatness / inaccuracy of a user's finger in points.
static const int kFingerTolerance = 10;

bool OverlapsOnYAxis(Rect a, Rect b) {
  return !(a.Top() < b.Bottom() || b.Top() < a.Bottom());
}

bool operator<(Candidate const& a, Candidate const& b) {
  if (a.line_index_ == b.line_index_) {
    return a.char_index_ < b.char_index_;
  }
  return a.line_index_ < b.line_index_;
}

Rect UnicodeCharacter::GetRect() const { return rect_; }

unsigned int UnicodeCharacter::GetCodePoint() const { return code_point_; }

bool UnicodeCharacter::IsEOL() const {
  const unsigned int kUnicodeNewlines[] = {0xA,  0xB,    0xC,    0xD,
                                           0X85, 0x2028, 0x2029, 0};
  const unsigned int* first = kUnicodeNewlines;
  const unsigned int* last =
      kUnicodeNewlines + ABSL_ARRAYSIZE(kUnicodeNewlines);
  return std::find(first, last, code_point_) != last;
}

float UnicodeCharacter::Left() const { return rect_.Left(); }
float UnicodeCharacter::Right() const { return rect_.Right(); }

void UnicodeCharacter::ExpandVerticallyToFit(float top, float bottom) {
  rect_ = Rect(rect_.to.x, bottom, rect_.from.x, top);
}

void UnicodeCharacter::ExpandRectWidth(float distance) {
  if (distance < 0) {
    rect_ = Rect(rect_.Left() + distance, rect_.Bottom(), rect_.Right(),
                 rect_.Top());
  } else {
    rect_ = Rect(rect_.Left(), rect_.Bottom(), rect_.Right() + distance,
                 rect_.Top());
  }
}

Rect Line::GetRect() const { return rect_; }

float Line::Top() const { return rect_.Top(); }
float Line::Bottom() const { return rect_.Bottom(); }
float Line::Left() const { return rect_.Left(); }
float Line::Right() const { return rect_.Right(); }

void Line::AddChar(UnicodeCharacter uc) {
  unichars_.push_back(uc);
  if (rect_.Empty()) {
    rect_ = uc.GetRect();
  }
  rect_ = rect_.Join(uc.GetRect());
}

std::string Line::ToString() const {
  std::vector<uint32_t> utf32;
  utf32.reserve(unichars_.size());
  for (const auto& c : unichars_) {
    utf32.emplace_back(c.GetCodePoint());
  }
  return Substitute("$0($1)", rect_, internal::Utf32ToUtf8(utf32));
}

TextPage::TextPage(FPDF_TEXTPAGE text_page) : text_page_(text_page) {}

int TextPage::CharCount() const {
  return FPDFText_CountChars(text_page_.get());
}

int TextPage::CodePointAt(int index) const {
  return FPDFText_GetUnicode(text_page_.get(), index);
}

StatusOr<Rect> TextPage::CharRectAt(int index) const {
  double left = 0;
  double right = 0;
  double top = 0;
  double bottom = 0;

  if (FPDFText_GetCharBox(text_page_.get(), index, &left, &right, &bottom,
                          &top)) {
    return Rect(left, bottom, right, top);
  }
  return ErrorStatus(StatusCode::OUT_OF_RANGE,
                     "text_page is invalid or index is out of bounds");
}

StatusOr<UnicodeCharacter> TextPage::UnicodeCharacterAt(int index) const {
  if (index < 0 || index >= CharCount()) {
    return ErrorStatus(StatusCode::OUT_OF_RANGE, "$0 is out of range [0, $1]",
                       index, CharCount() - 1);
  }
  int code_point = CodePointAt(index);
  INK_ASSIGN_OR_RETURN(Rect r, CharRectAt(index));
  return UnicodeCharacter(code_point, r);
}

StatusOr<Line> TextPage::LineAt(int index) const {
  if (index < 0 || index >= lines_.size()) {
    return ErrorStatus(StatusCode::OUT_OF_RANGE, "$0 is out of range [0, $1]",
                       index, lines_.size() - 1);
  }
  return lines_[index];
}

int TextPage::LineCount() const { return lines_.size(); }

Status TextPage::GenerateIndex() {
  int num_chars = CharCount();
  Line curr_line;
  for (int i = 0; i < num_chars; ++i) {
    bool last_char = i == num_chars - 1;
    INK_ASSIGN_OR_RETURN(UnicodeCharacter uc, UnicodeCharacterAt(i));
    if (!uc.IsEOL()) {
      curr_line.AddChar(uc);
    }
    bool is_eol = uc.IsEOL() || last_char;
    if (!is_eol && uc.GetCodePoint() == kLineSplittingHyphen) {
      INK_ASSIGN_OR_RETURN(Rect next_char_rect, CharRectAt(i + 1));
      is_eol = !OverlapsOnYAxis(uc.GetRect(), next_char_rect);
    }
    if (is_eol) {
      if (!last_char && uc.GetCodePoint() == 0xD) {
        if (CodePointAt(i + 1) == 0xA) {
          ++i;
        }
      }
      ExpandCharactersToFillLine(&curr_line);
      lines_.push_back(curr_line);
      curr_line = Line();
    }
  }
  return OkStatus();
}

void TextPage::ExpandCharactersToFillLine(Line* line) {
  float line_bottom = line->Bottom();
  float line_top = line->Top();
  for (int c = 0; c < line->unichars_.size(); ++c) {
    line->unichars_[c].ExpandVerticallyToFit(line_top, line_bottom);
    if (c != line->unichars_.size() - 1) {
      // Removes gap between characters by expanding the width of both
      // UnicodeCharacter Rects halfway towards the center of the gap. This
      // makes sure that a point between two characters is always contained by a
      // UnicodeCharacter Rect.
      float half_distance =
          (line->unichars_[c + 1].Left() - line->unichars_[c].Right()) / 2.0;
      line->unichars_[c].ExpandRectWidth(half_distance);
      line->unichars_[c + 1].ExpandRectWidth(-half_distance);
    }
  }
}

// How close to a line do you have to be in order to be "above" it?
static constexpr float kVerticalAboveSlopFactor = 2.0;

// How close to a line do you have to be in order to be "below" it?
static constexpr float kVerticalBelowSlopFactor = .5;

std::vector<Candidate> TextPage::CandidatesAt(glm::vec2 point) const {
  SLOG(SLOG_PDF, "Finding candidates at $0", point);

  for (int i = 0; i < LineCount(); ++i) {
    const Line& line = lines_[i];

    double margin = line.GetRect().Height() * kLineHitMarginFactor;
    double left_margin = line.Left() - margin;
    double right_margin = line.Right() + margin;
    SLOG(SLOG_PDF, "  Considering line $0 at $1 with margin $2", i,
         line.GetRect(), margin);

    const auto x = point.x;
    const auto y = point.y;
    if (x < left_margin || x > right_margin) {
      SLOG(SLOG_PDF, "    $0 is outside left $1 or right $2", x, left_margin,
           right_margin);
      continue;
    }

    const auto line_bottom = line.Bottom();
    const auto line_top = line.Top();

    // in a line, find the char
    if (line_top >= y && line_bottom <= y) {
      for (int j = 0; j < line.unichars_.size(); ++j) {
        const UnicodeCharacter& c = line.unichars_[j];
        // expand the hitbox off the ends of the line by a margin
        Rect hitbox = c.GetRect();
        if (j == 0) {
          hitbox = Rect(left_margin, line_bottom, hitbox.Right(), line_top);
        }
        if (j == line.unichars_.size() - 1) {
          hitbox = Rect(hitbox.Left(), line_bottom, right_margin, line_top);
        }
        if (hitbox.Contains(point)) {
          SLOG(SLOG_PDF, "    $0 is in hitbox of char $1", point, j);
          // in this char
          if (x >= c.GetRect().Center().x) {
            // on the right half of the char
            auto end_of_this_char = Candidate(i, j, Candidate::R);
            if (j == line.unichars_.size() - 1) {
              return {end_of_this_char};
            } else {
              return {end_of_this_char, Candidate(i, j + 1, Candidate::L)};
            }
          }
          // on the left half of the char
          auto start_of_this_char = Candidate(i, j, Candidate::L);
          if (j == 0) {
            // first char, has to be to the left of this one
            return {start_of_this_char};
          }
          // either the right side of the previous char, or the left of this
          return {Candidate(i, j - 1, Candidate::R), start_of_this_char};
        }
      }
      SLOG(SLOG_ERROR, "In a line, but not in any character!");
    }

    const float vertical_above_slop =
        kVerticalAboveSlopFactor * line.GetRect().Height();
    // above the line
    if (y > line_top && y < line_top + vertical_above_slop) {
      SLOG(SLOG_PDF, "    $0 is above but within $1 of top", point,
           vertical_above_slop);
      if (i == 0) {
        // this is the top line, so only one candidate possible
        return {Candidate(i, 0, Candidate::L)};
      } else {
        const auto previous_line = lines_[i - 1];
        if (y < previous_line.Bottom() || previous_line.Bottom() < line_top) {
          // either the last char of the previous line, or the first of this
          return {Candidate(i - 1, previous_line.unichars_.size() - 1,
                            Candidate::R),
                  Candidate(i, 0, Candidate::L)};
        }
      }
    }

    // below the line
    const float vertical_below_slop =
        kVerticalBelowSlopFactor * line.GetRect().Height();
    if (y < line_bottom && y > line_bottom - vertical_below_slop) {
      SLOG(SLOG_PDF, "    $0 is below but within $1 of bottom", point,
           vertical_below_slop);
      auto end_of_this_line =
          Candidate(i, line.unichars_.size() - 1, Candidate::R);
      if (i == lines_.size() - 1) {
        // last line, so only one candidate possible
        return {end_of_this_line};
      } else {
        const auto next_line = lines_[i + 1];
        if (y > next_line.Top() || next_line.Top() > line_top) {
          // This is the bottom line of a column, so the "above" case
          // won't catch this. Either the last char of this line or the
          // first of the next (the top of the next column).
          // or
          // This is a case where the point is below this line and above the
          // next line, but not within the margins of the next line
          return {end_of_this_line, Candidate(i + 1, 0, Candidate::L)};
        }
      }
    }
    SLOG(SLOG_PDF, "    $0 is outside vertical tolerance ($1, $2)", point.y,
         line_bottom - vertical_below_slop, line_top + vertical_above_slop);
  }

  return {};
}

void TextPage::SortCandidatesInPlace(std::vector<Candidate>* ca,
                                     std::vector<Candidate>* cb) const {
  if (cb->at(0) < ca->at(0)) std::swap(*ca, *cb);
}

void TextPage::AppendLineChunk(const Candidate& a, const Candidate& b,

                               std::vector<Rect>* selection,
                               glm::mat4 transform) {
  const Line& line = lines_[a.line_index_];
  const UnicodeCharacter& ca = line.unichars_[a.char_index_];
  const UnicodeCharacter& cb = line.unichars_[b.char_index_];

  if (a.char_index_ == b.char_index_) {
    // Both endpoints are in the same char.
    if (a.direction_ == Candidate::L && b.direction_ == Candidate::R) {
      // Only add the char rect if the whole char is touched.
      selection->push_back(geometry::Transform(ca.GetRect(), transform));
    }
  } else {
    selection->push_back(geometry::Transform(
        Rect(ca.Left(), line.Top(), cb.Right(), line.Bottom()), transform));
  }
}

void TextPage::GetSelectionRects(glm::vec2 p, glm::vec2 q,
                                 glm::mat4 page_to_world,
                                 std::vector<Rect>* out) {
  std::vector<Candidate> pc = CandidatesAt(p);
  std::vector<Candidate> qc = CandidatesAt(q);

  if (pc == qc || pc.empty() || qc.empty()) {
    return;
  }

  // Sort candidates.
  SortCandidatesInPlace(&pc, &qc);

  // pc is "first", qc is "second."
  const Candidate& a = pc[pc.size() - 1];
  const Candidate& b = qc[0];

  if (a.line_index_ == b.line_index_) {
    // Both endpoints are on the same line.
    AppendLineChunk(a, b, out, page_to_world);
    return;
  }

  AppendLineChunk(
      a,
      Candidate(a.line_index_, lines_[a.line_index_].unichars_.size() - 1,
                Candidate::R),
      out, page_to_world);

  for (int i = a.line_index_ + 1; i < b.line_index_; ++i) {
    // Append the whole line.
    out->push_back(geometry::Transform(lines_[i].GetRect(), page_to_world));
  }

  AppendLineChunk(Candidate(b.line_index_, 0, Candidate::L), b, out,
                  page_to_world);
}

void TextPage::GetSelectionRectsToEnd(glm::vec2 from, glm::mat4 page_to_world,
                                      std::vector<Rect>* out) {
  if (LineCount() == 0) {
    return;
  }
  const Line& line = lines_[LineCount() - 1];
  // Use last character of the last line's right bottom point as last_char.
  glm::vec2 last_char_point(
      line.unichars_[line.unichars_.size() - 1].GetRect().Rightbottom());
  GetSelectionRects(last_char_point, from, page_to_world, out);
}

void TextPage::GetSelectionRectsFromBeginning(glm::vec2 to,
                                              glm::mat4 page_to_world,
                                              std::vector<Rect>* out) {
  if (LineCount() == 0) {
    return;
  }
  const Line& line = lines_[0];
  // using first char of first line's left bottom point as first_char
  glm::vec2 first_char_point(line.unichars_[0].GetRect().Lefttop());
  GetSelectionRects(first_char_point, to, page_to_world, out);
}

void TextPage::GetSelectionRects(glm::mat4 page_to_world,
                                 std::vector<Rect>* out) {
  for (auto& line : lines_) {
    out->push_back(geometry::Transform(line.GetRect(), page_to_world));
  }
}

std::string TextPage::GetSelectionTextImpl(glm::vec2 a, glm::vec2 b) const {
  int start_index = FPDFText_GetCharIndexAtPos(
      text_page_.get(), a.x, a.y, kFingerTolerance, kFingerTolerance);

  int end_index = FPDFText_GetCharIndexAtPos(
      text_page_.get(), b.x, b.y, kFingerTolerance, kFingerTolerance);

  if (start_index > end_index) std::swap(start_index, end_index);

  std::vector<char16_t> selection;
  int char_count = (end_index - start_index) + 1;
  selection.resize(char_count + 1);
  FPDFText_GetText(text_page_.get(), start_index, char_count,
                   reinterpret_cast<uint16_t*>(selection.data()));
  if (!selection.empty()) {
    selection.pop_back();
  }

  return internal::Utf16LEToUtf8(selection);
}

std::string TextPage::GetSelectionText(glm::vec2 p, glm::vec2 q) const {
  std::vector<Candidate> pc = CandidatesAt(p);
  std::vector<Candidate> qc = CandidatesAt(q);

  if (pc == qc || pc.empty() || qc.empty()) {
    return "";
  }

  // sort candidates
  SortCandidatesInPlace(&pc, &qc);

  // pc is "first", qc is "second"
  const Candidate& a = pc[pc.size() - 1];
  const Candidate& b = qc[0];

  glm::vec2 start =
      lines_[a.line_index_].unichars_[a.char_index_].GetRect().Center();

  glm::vec2 end =
      lines_[b.line_index_].unichars_[b.char_index_].GetRect().Center();

  return GetSelectionTextImpl(start, end);
}

std::string TextPage::GetSelectionTextToEnd(glm::vec2 from) const {
  std::string selection;
  if (LineCount() == 0) {
    return selection;
  }
  const Line& line = lines_[LineCount() - 1];
  // using last character of the last line's right bottom point as last_char
  glm::vec2 last_char_point(
      line.unichars_[line.unichars_.size() - 1].GetRect().Rightbottom());
  return GetSelectionText(last_char_point, from);
}

std::string TextPage::GetSelectionTextFromBeginning(glm::vec2 to) const {
  std::string selection;
  if (LineCount() == 0) {
    return selection;
  }
  const Line& line = lines_[0];
  // using first char of first line's left top point as first_char
  glm::vec2 first_char_point(line.unichars_[0].GetRect().Lefttop());
  return GetSelectionText(first_char_point, to);
}

std::string TextPage::GetText() const {
  std::string selection;
  if (LineCount() == 0) {
    return selection;
  }
  const Line& line = lines_[0];
  // using first char of first line's left top point as first_char
  glm::vec2 first_char_point(line.unichars_[0].GetRect().Lefttop());
  return GetSelectionTextToEnd(first_char_point);
}

bool TextPage::IsInText(glm::vec2 p) const {
  auto candidates = CandidatesAt(p);
  return !candidates.empty();
}

Rect TextPage::CandidateRect(const Candidate& c) const {
  if (c.line_index_ < 0 || c.line_index_ >= LineCount()) return Rect();
  const Line& line = lines_[c.line_index_];
  if (c.char_index_ < 0 || c.char_index_ >= line.unichars_.size())
    return Rect();
  return line.unichars_[c.char_index_].GetRect();
}

}  // namespace pdf
}  // namespace ink
