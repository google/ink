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

#include "ink/pdf/page_object_mark.h"

#include "third_party/pdfium/public/fpdf_edit.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/pdf/internal.h"

namespace {
// No string params > 10MB
constexpr size_t kMaxStringParamSize = 10000000;
}  // namespace

namespace ink {
namespace pdf {

Status PageObjectMark::GetName(std::string* out) const {
  return internal::FetchUtf16StringAsUtf8(
      [this](void* buf, size_t len) {
        unsigned long out_buflen;
        FPDFPageObjMark_GetName(mark_, buf, len, &out_buflen);
        return static_cast<size_t>(out_buflen);
      },
      out);
}

Status PageObjectMark::GetIntParam(absl::string_view key, int* value) const {
  INK_RETURN_UNLESS(ExpectParamType(key, FPDF_OBJECT_NUMBER));

  std::string skey = static_cast<std::string>(key);
  if (!FPDFPageObjMark_GetParamIntValue(mark_, skey.c_str(), value)) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not fetch int param $0",
                       key);
  }
  return OkStatus();
}

Status PageObjectMark::SetIntParam(absl::string_view key, int value) {
  std::string skey = static_cast<std::string>(key);
  if (!FPDFPageObjMark_SetIntParam(owning_document_, owning_pageobject_, mark_,
                                   skey.c_str(), value)) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not set param $0.", key);
  }
  return OkStatus();
}

Status PageObjectMark::GetStringParam(absl::string_view key,
                                      std::string* value) const {
  INK_RETURN_UNLESS(ExpectParamType(key, FPDF_OBJECT_STRING));

  std::string skey = static_cast<std::string>(key);
  unsigned long expected_size;
  if (!FPDFPageObjMark_GetParamBlobValue(mark_, skey.c_str(), nullptr, 0,
                                         &expected_size)) {
    return ErrorStatus(StatusCode::INTERNAL,
                       "Could not determine size of string param $0", key);
  }
  if (expected_size > kMaxStringParamSize) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Cannot reserve more than $0 bytes for string param $1; "
                       "$2 bytes requested.",
                       kMaxStringParamSize, key, expected_size);
  }
  // 0-fill result
  value->resize(expected_size, 0);
  unsigned long outlen;
  if (!FPDFPageObjMark_GetParamBlobValue(mark_, skey.c_str(), &((*value)[0]),
                                         expected_size, &outlen)) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not fetch string param $0",
                       key);
  }
  if (outlen != expected_size) {
    return ErrorStatus(
        StatusCode::INTERNAL,
        "Expected length for param $0 was $1 bytes, got $2 bytes.", key,
        expected_size, outlen);
  }
  return OkStatus();
}

Status PageObjectMark::SetStringParam(absl::string_view key,
                                      absl::string_view value) {
  std::string skey = static_cast<std::string>(key);
  std::string svalue = static_cast<std::string>(value);
  if (!FPDFPageObjMark_SetBlobParam(
          owning_document_, owning_pageobject_, mark_, skey.c_str(),
          &svalue[0],  // fpdfium API is not const-safe, so we cannot pass
                       // svalue.data().
          svalue.length())) {
    return ErrorStatus(StatusCode::INTERNAL, "Could not set param $0.", key);
  }
  return OkStatus();
}

namespace {
std::string FpdfTypeName(FPDF_OBJECT_TYPE type) {
  switch (type) {
    case FPDF_OBJECT_STRING:
      return "FPDF_OBJECT_STRING";
    case FPDF_OBJECT_NUMBER:
      return "FPDF_OBJECT_NUMBER";
    case FPDF_OBJECT_UNKNOWN:
      return "FPDF_OBJECT_UNKNOWN";
    case FPDF_OBJECT_ARRAY:
      return "FPDF_OBJECT_ARRAY";
    case FPDF_OBJECT_BOOLEAN:
      return "FPDF_OBJECT_BOOLEAN";
    case FPDF_OBJECT_NAME:
      return "FPDF_OBJECT_NAME";
    case FPDF_OBJECT_DICTIONARY:
      return "FPDF_OBJECT_DICTIONARY";
    case FPDF_OBJECT_STREAM:
      return "FPDF_OBJECT_STREAM";
    case FPDF_OBJECT_NULLOBJ:
      return "FPDF_OBJECT_NULLOBJ";
    case FPDF_OBJECT_REFERENCE:
      return "FPDF_OBJECT_REFERENCE";
    default:
      return Substitute("<unexpected FPDF_OBJECT_TYPE $0>", type);
  }
}
}  // namespace

Status PageObjectMark::ExpectParamType(absl::string_view key,
                                       FPDF_OBJECT_TYPE expected_type) const {
  std::string skey = static_cast<std::string>(key);
  auto const type = FPDFPageObjMark_GetParamValueType(mark_, skey.c_str());
  if (type == FPDF_OBJECT_UNKNOWN) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "No param found for key <$0>.", key);
  }
  if (type != expected_type) {
    return ErrorStatus(StatusCode::INVALID_ARGUMENT,
                       "Param found for key <$0> is $1, not the expected $2.",
                       key, FpdfTypeName(type), FpdfTypeName(expected_type));
  }
  return OkStatus();
}

}  // namespace pdf
}  // namespace ink
