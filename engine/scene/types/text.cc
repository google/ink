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

#include "ink/engine/scene/types/text.h"
#include "ink/engine/colors/colors.h"
#include "ink/engine/util/proto/serialize.h"

namespace ink {
namespace text {

using status::InvalidArgument;

// Arbitrary upper bounds on UTF8-encoded text string.
static constexpr int kMaxTextSize = 10000;

// You can't name a font with more characters than this.
static constexpr int kMaxFontNameSize = 100;

// You can't have an asset id with more characters than this.
static constexpr int kMaxAssetIdSize = 100;

static PostscriptFont protoPostscriptToPostscript(
    const proto::text::PostscriptFont& proto) {
  switch (proto) {
    case proto::text::PostscriptFont::DEFAULT_POSTSCRIPT_FONT:
    case proto::text::PostscriptFont::HELVETICA_REGULAR:
      return PostscriptFont::kHelveticaRegular;
    case proto::text::PostscriptFont::HELVETICA_BOLD:
      return PostscriptFont::kHelveticaBold;
    case proto::text::PostscriptFont::HELVETICA_OBLIQUE:
      return PostscriptFont::kHelveticaOblique;
    case proto::text::PostscriptFont::HELVETICA_BOLD_OBLIQUE:
      return PostscriptFont::kHelveticaBoldOblique;
    case proto::text::PostscriptFont::TIMES_REGULAR:
      return PostscriptFont::kTimesRegular;
    case proto::text::PostscriptFont::TIMES_BOLD:
      return PostscriptFont::kTimesBold;
    case proto::text::PostscriptFont::TIMES_OBLIQUE:
      return PostscriptFont::kTimesOblique;
    case proto::text::PostscriptFont::TIMES_BOLD_OBLIQUE:
      return PostscriptFont::kTimesBoldOblique;
    case proto::text::PostscriptFont::COURIER_REGULAR:
      return PostscriptFont::kCourierRegular;
    case proto::text::PostscriptFont::COURIER_BOLD:
      return PostscriptFont::kCourierBold;
    case proto::text::PostscriptFont::COURIER_OBLIQUE:
      return PostscriptFont::kCourierOblique;
    case proto::text::PostscriptFont::COURIER_BOLD_OBLIQUE:
      return PostscriptFont::kCourierBoldOblique;
    case proto::text::PostscriptFont::SYMBOL:
      return PostscriptFont::kSymbol;
    default:
      return PostscriptFont::kUndefined;
  }
}

static proto::text::PostscriptFont postscriptFontToProto(
    const PostscriptFont& font) {
  switch (font) {
    case PostscriptFont::kHelveticaRegular:
      return proto::text::PostscriptFont::HELVETICA_REGULAR;
    case PostscriptFont::kHelveticaBold:
      return proto::text::PostscriptFont::HELVETICA_BOLD;
    case PostscriptFont::kHelveticaOblique:
      return proto::text::PostscriptFont::HELVETICA_OBLIQUE;
    case PostscriptFont::kHelveticaBoldOblique:
      return proto::text::PostscriptFont::HELVETICA_BOLD_OBLIQUE;
    case PostscriptFont::kTimesRegular:
      return proto::text::PostscriptFont::TIMES_REGULAR;
    case PostscriptFont::kTimesBold:
      return proto::text::PostscriptFont::TIMES_BOLD;
    case PostscriptFont::kTimesOblique:
      return proto::text::PostscriptFont::TIMES_OBLIQUE;
    case PostscriptFont::kTimesBoldOblique:
      return proto::text::PostscriptFont::TIMES_BOLD_OBLIQUE;
    case PostscriptFont::kCourierRegular:
      return proto::text::PostscriptFont::COURIER_REGULAR;
    case PostscriptFont::kCourierBold:
      return proto::text::PostscriptFont::COURIER_BOLD;
    case PostscriptFont::kCourierOblique:
      return proto::text::PostscriptFont::COURIER_OBLIQUE;
    case PostscriptFont::kCourierBoldOblique:
      return proto::text::PostscriptFont::COURIER_BOLD_OBLIQUE;
    case PostscriptFont::kSymbol:
      return proto::text::PostscriptFont::SYMBOL;
    default:
      SLOG(SLOG_ERROR, "Did not expect an undefined postscript font");
      return proto::text::PostscriptFont::DEFAULT_POSTSCRIPT_FONT;
  }
}

static Alignment protoAlignmentToAlignment(
    const proto::text::Alignment& proto_alignment) {
  switch (proto_alignment) {
    case proto::text::Alignment::DEFAULT_ALIGNMENT:
    case proto::text::Alignment::LEFT:
      return Alignment::kLeft;
    case proto::text::Alignment::CENTERED:
      return Alignment::kCentered;
    case proto::text::Alignment::RIGHT:
      return Alignment::kRight;
    default:
      return Alignment::kUndefined;
  }
}

Status TextSpec::ReadFromProto(const proto::text::Text& unsafe_proto,
                               TextSpec* result) {
  if (!unsafe_proto.has_text() || unsafe_proto.text().size() > kMaxTextSize) {
    return InvalidArgument("proto has missing or overlong text");
  }
  result->text_utf8 = unsafe_proto.text();

  if (!unsafe_proto.has_font()) {
    return InvalidArgument("no font specified in text proto");
  }
  const auto& font = unsafe_proto.font();
  // Enforce pseudo-oneof.
  if (font.has_resource_id() + font.has_asset_id() + font.has_name() +
          font.has_postscript_font() !=
      1) {
    return InvalidArgument("exactly one font must be specified in proto");
  }
  if (font.has_postscript_font()) {
    result->font_tag = font_type_tag::kPostscriptFont;
    result->font = protoPostscriptToPostscript(font.postscript_font());
    if (absl::get<PostscriptFont>(result->font) == PostscriptFont::kUndefined) {
      return InvalidArgument("cannot interpret postscript font $0 in proto",
                             static_cast<int>(font.postscript_font()));
    }
  } else if (font.has_name()) {
    absl::string_view name(font.name());
    if (name.empty() || name.size() > kMaxFontNameSize) {
      return InvalidArgument("font name \"$0\" is empty or too long", name);
    }
    result->font_tag = font_type_tag::kName;
    result->font = static_cast<std::string>(name);
  } else if (font.has_asset_id()) {
    absl::string_view asset_id(font.asset_id());
    if (asset_id.empty() || asset_id.size() > kMaxAssetIdSize) {
      return InvalidArgument("asset id \"$0\" is empty or too long", asset_id);
    }
    result->font_tag = font_type_tag::kAssetId;
    result->font = static_cast<std::string>(asset_id);
  } else if (font.has_resource_id()) {
    result->font_tag = font_type_tag::kResourceId;
    result->font = font.resource_id();
  }

  if (!unsafe_proto.has_font_size_fraction()) {
    return InvalidArgument("no font size specified in proto");
  }
  const auto& font_size_fraction = unsafe_proto.font_size_fraction();
  if (font_size_fraction <= 0) {
    return InvalidArgument("invalid font size $0 specified in proto",
                           font_size_fraction);
  }
  result->font_size_fraction = font_size_fraction;

  if (unsafe_proto.has_rgba()) {
    result->color = UintToVec4RGBA(unsafe_proto.rgba());
  } else {
    SLOG(SLOG_WARNING, "using default color for text");
  }

  if (unsafe_proto.has_alignment()) {
    result->alignment = protoAlignmentToAlignment(unsafe_proto.alignment());
    if (result->alignment == Alignment::kUndefined) {
      return InvalidArgument("cannot interpret proto alignment $0",
                             static_cast<int>(unsafe_proto.alignment()));
    }
  } else {
    SLOG(SLOG_WARNING, "using default alignment for text");
  }

  if (unsafe_proto.has_shadow() &&
      unsafe_proto.shadow().radius_fraction() > 0) {
    result->shadow_color = UintToVec4RGBA(unsafe_proto.shadow().rgba());
    result->shadow_radius_fraction = unsafe_proto.shadow().radius_fraction();
    result->shadow_dx_fraction = unsafe_proto.shadow().dx_fraction();
    result->shadow_dy_fraction = unsafe_proto.shadow().dy_fraction();
  }

  if (unsafe_proto.has_layout()) {
    result->layout = unsafe_proto.layout();
  }

  return OkStatus();
}

void TextSpec::WriteToProto(proto::text::Text* proto, const TextSpec& text) {
  proto->Clear();
  proto->set_text(text.text_utf8);
  switch (text.font_tag) {
    case font_type_tag::kPostscriptFont:
      proto->mutable_font()->set_postscript_font(
          postscriptFontToProto(absl::get<PostscriptFont>(text.font)));
      break;
    case font_type_tag::kResourceId:
      proto->mutable_font()->set_resource_id(absl::get<uint32_t>(text.font));
      break;
    case font_type_tag::kAssetId:
      proto->mutable_font()->set_asset_id(absl::get<std::string>(text.font));
      break;
    case font_type_tag::kName:
      proto->mutable_font()->set_name(absl::get<std::string>(text.font));
      break;
  }
  proto->set_font_size_fraction(text.font_size_fraction);
  proto->set_rgba(Vec4ToUintRGBA(text.color));
  switch (text.alignment) {
    case Alignment::kLeft:
      proto->set_alignment(proto::text::Alignment::LEFT);
      break;
    case Alignment::kCentered:
      proto->set_alignment(proto::text::Alignment::CENTERED);
      break;
    case Alignment::kRight:
      proto->set_alignment(proto::text::Alignment::RIGHT);
      break;
    default:
      SLOG(SLOG_ERROR, "Did not expect an undefined alignment");
      proto->set_alignment(proto::text::Alignment::DEFAULT_ALIGNMENT);
      break;
  }
  if (text.shadow_radius_fraction > 0) {
    proto->mutable_shadow()->set_rgba(Vec4ToUintRGBA(text.shadow_color));
    proto->mutable_shadow()->set_radius_fraction(text.shadow_radius_fraction);
    proto->mutable_shadow()->set_dx_fraction(text.shadow_dx_fraction);
    proto->mutable_shadow()->set_dy_fraction(text.shadow_dy_fraction);
  }

  // Only write a layout if there is one.
  if (text.layout) {
    *proto->mutable_layout() = *text.layout;
  }
}

}  // namespace text
}  // namespace ink
