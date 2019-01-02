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

#ifndef INK_ENGINE_PUBLIC_TYPES_STATUS_H_
#define INK_ENGINE_PUBLIC_TYPES_STATUS_H_

#include <ostream>
#include <string>

#include "third_party/absl/base/attributes.h"
#include "third_party/absl/strings/string_view.h"
#include "third_party/absl/strings/substitute.h"
#include "ink/engine/util/dbg/log.h"
#include "ink/engine/util/dbg/str.h"

#ifdef SKETCHOLOGY_STATUS_PROVIDE_GOOGLE3_CONVERSION
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#endif

namespace ink {

enum class StatusCode {
  OK,
  UNKNOWN,
  INCOMPLETE,
  INVALID_ARGUMENT,
  FAILED_PRECONDITION,
  NOT_FOUND,
  ALREADY_EXISTS,
  OUT_OF_RANGE,
  UNIMPLEMENTED,
  INTERNAL,
};

// A Status value represents the result of an operation or API call.
class Status {
 public:
  Status() : Status(StatusCode::UNKNOWN, "") {}
  explicit Status(absl::string_view message)
      : Status(StatusCode::UNKNOWN, message) {}
  explicit Status(StatusCode code) : Status(code, "") {}

  // If `code == StatusCode::OK`, `message` is ignored and an object identical
  // to an OK status is constructed.
  Status(StatusCode code, absl::string_view message)
      : code_(code), message_(message) {
    if (code == StatusCode::OK) {
      message_.clear();
    }
  }

  ABSL_MUST_USE_RESULT inline bool ok() const {
    return code_ == StatusCode::OK;
  }

  inline std::string error_message() const { return message_; }

  inline std::string ToString() const { return ok() ? "OK" : error_message(); }

  StatusCode code() const { return code_; }

  inline explicit operator bool() const { return ok(); }

  inline Status(const Status& s) = default;
  inline Status& operator=(const Status& x) = default;

  Status WithoutMessage() const { return Status(code_); }

  // This lets the user of some function returning a Status to ignore the
  // result. Use at your own risk.
  inline void IgnoreError() const {
    if (!ok()) SLOG(SLOG_ERROR, "$0", *this);
  }

  inline bool operator==(const Status& x) const {
    return x.code_ == code_ && x.message_ == message_;
  }
  inline bool operator!=(const Status& x) const { return !(x == *this); }

#ifdef SKETCHOLOGY_STATUS_PROVIDE_GOOGLE3_CONVERSION
  // Deliberate implicit conversion for use in unit test CHECK_OK and the like.
  inline operator ::util::Status() const {
    return ok() ? ::util::OkStatus() : ::util::UnknownError(message_);
  }
#endif

 private:
  StatusCode code_;
  std::string message_;
};

inline Status OkStatus() { return Status(StatusCode::OK); }
inline Status ErrorStatus(absl::string_view message) { return Status(message); }

// Convenience methods to avoid every caller having to use Substitute
// themselves.
template <typename... T>
Status ErrorStatus(absl::string_view format, const T&... args) {
  return Status(absl::Substitute(format, Str(args)...));
}

template <typename... T>
Status ErrorStatus(StatusCode code, absl::string_view format,
                   const T&... args) {
  return Status(code, absl::Substitute(format, Str(args)...));
}

namespace status {
template <typename... T>
Status InvalidArgument(absl::string_view format, const T&... args) {
  return Status(StatusCode::INVALID_ARGUMENT,
                absl::Substitute(format, Str(args)...));
}
}  // namespace status

inline std::ostream& operator<<(std::ostream& os, const Status& x) {
  os << x.ToString();
  return os;
}

using StatusPredicate = std::function<bool(const Status&)>;

// Predefined StatusPredicates.
namespace status {
inline bool IsIncomplete(const Status& s) {
  return s.code() == StatusCode::INCOMPLETE;
}
inline bool IsInvalidArgument(const Status& s) {
  return s.code() == StatusCode::INVALID_ARGUMENT;
}
inline bool IsNotFound(const Status& s) {
  return s.code() == StatusCode::NOT_FOUND;
}
inline bool IsAlreadyExists(const Status& s) {
  return s.code() == StatusCode::ALREADY_EXISTS;
}
}  // namespace status

#define INK_RETURN_UNLESS(STATUS_EXPRESSION) \
  do {                                       \
    auto _status_ = (STATUS_EXPRESSION);     \
    if (!_status_) {                         \
      return _status_;                       \
    }                                        \
  } while (0);

#define INK_RETURN_FALSE_UNLESS(STATUS_EXPRESSION) \
  do {                                             \
    auto _status_ = (STATUS_EXPRESSION);           \
    if (!_status_) {                               \
      return false;                                \
    }                                              \
  } while (0);

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_TYPES_STATUS_H_
