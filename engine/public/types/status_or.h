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

#ifndef INK_ENGINE_PUBLIC_TYPES_STATUS_OR_H_
#define INK_ENGINE_PUBLIC_TYPES_STATUS_OR_H_

#include "third_party/absl/base/optimization.h"
#include "third_party/absl/types/variant.h"
#include "ink/engine/public/types/status.h"
#include "ink/engine/util/dbg/errors.h"

namespace ink {

// A StatusOr will contain a value of type T iff status_or.ok() == true.  If the
// StatusOr object is non-ok, then it will contain a non-ok Status object.
//
// Requesting a value from a failed StatusOr or requesting an error message or
// status code from a successful StatusOr is a programmer error and will
// expect-fail.
//
// Example usage:
//
//   extern void DoSomethingWithTheValue(int val);
//
//   Status RequestThatMayFail(param0) {
//     StatusOr<int> status_or = RequestIntThatMayFail(param0, param1);
//
//     // NOTE: a StatusOr will contextually convert to bool.
//     if (!status_or) return status_or.status();
//
//     DoSomethingWithTheValue(status_or.ValueOrDie());
//     return status_or.status();
//   }
//
//   StatusOr<int> AnotherFuncThatMayFail(int positive_integer) {
//     Status st = CheckForPositiveInteger(positive_integer);
//
//     // NOTE: a Status will auto-convert to a StatusOr for an error Status.
//     if (!st.ok()) return st;
//
//     // NOTE: a value will auto-convert to a StatusOr when returned.
//     return positive_integer + 20;
//   }
//
// NOTE: StatusOr will also act a boolean value, returning ok(), in contexts
//       that cause an explicit bool conversion.
//
template <typename T>
class StatusOr {
 public:
  // Create a new successful StatusOr with the specified value.
  //
  // This is an implicit conversion to allow direct return of a value
  // as a StatusOr.
  StatusOr(const T &value) : values_(value) {}
  StatusOr(T &&value) : values_(std::move(value)) {}

  // Create a new Error StatusOr from a Status.
  //
  // This is an implicit conversion to allow direct return of an error
  // status to be converted to a StatusOr.
  StatusOr(const Status &st);
  StatusOr(Status &&st);

  StatusOr() = delete;
  ~StatusOr() = default;

  // Copy constructors and assignment operators.
  StatusOr(const StatusOr<T> &other) = default;
  StatusOr<T> &operator=(const StatusOr<T> &other) = default;

  // Move constructors and assignment operator.
  StatusOr(StatusOr<T> &&other) = default;
  StatusOr<T> &operator=(StatusOr<T> &&other) = default;

  // ok() == true iff this contains a non-error value.
  bool ok() const { return values_.index() == 0; }

  // Contextual conversion to bool. Returns value of ok().
  explicit operator bool() const { return ok(); }

  // Safely convert from a StatusOr to a Status.
  Status status() const {
    return ok() ? OkStatus() : absl::get<Status>(values_);
  }

  // Returns a reference to our current value, or CHECK-fails if !this->ok(). If
  // you have already checked the status using this->ok() or operator bool(),
  // then you probably want to use operator*() or operator->() to access the
  // current value instead of ValueOrDie().
  //
  // Note: for value types that are cheap to copy, prefer simple code:
  //
  //   T value = statusor.ValueOrDie();
  //
  // Otherwise, if the value type is expensive to copy, but can be left
  // in the StatusOr, simply assign to a reference:
  //
  //   T& value = statusor.ValueOrDie();  // or `const T&`
  //
  // Otherwise, if the value type supports an efficient move, it can be
  // used as follows:
  //
  //   T value = std::move(statusor).ValueOrDie();
  //
  // The std::move on statusor instead of on the whole expression enables
  // warnings about possible uses of the statusor object after the move.
  const T &ValueOrDie() const &;
  T &ValueOrDie() &;
  const T &&ValueOrDie() const &&;
  T &&ValueOrDie() &&;

  // iff ok() != true, returns the error message. (expect-fail otherwise.)
  std::string error_message() const;
  // iff ok() != true, returns the error code. (expect-fail otherwise.)
  StatusCode code() const;

 private:
  void EnsureOk() const {
    if (!ok()) RUNTIME_ERROR("$0", status());
  }

  // The value or error.
  absl::variant<T, Status> values_;
};

// Creates a new StatusOr from an error Status.
// The error Status will be constructed from params.
//
// Passing arguments that would create an ok() Status will cause this to
// expect-fail.
template <typename T, typename... Us>
StatusOr<T> ErrorStatusOr(Us... params) {
  return StatusOr<T>(Status(params...));
}

template <typename T>
StatusOr<T>::StatusOr(const Status &st) : values_(st) {
  EXPECT(!st.ok());
}

template <typename T>
StatusOr<T>::StatusOr(Status &&st) : values_(std::move(st)) {
  EXPECT(!st.ok());
}

template <typename T>
const T &StatusOr<T>::ValueOrDie() const & {
  this->EnsureOk();
  return absl::get<T>(this->values_);
}

template <typename T>
T &StatusOr<T>::ValueOrDie() & {
  this->EnsureOk();
  return absl::get<T>(this->values_);
}

template <typename T>
const T &&StatusOr<T>::ValueOrDie() const && {
  this->EnsureOk();
  return absl::get<T>(std::move(values_));
}

template <typename T>
T &&StatusOr<T>::ValueOrDie() && {
  this->EnsureOk();
  return absl::get<T>(std::move(values_));
}

template <typename T>
std::string StatusOr<T>::error_message() const {
  EXPECT(!ok());
  return absl::get<Status>(values_).error_message();
}

template <typename T>
StatusCode StatusOr<T>::code() const {
  EXPECT(!ok());
  return absl::get<Status>(values_).code();
}

// Executes an expression `STATUS_OR_EXPRESSION` that returns an
// `ink::StatusOr<T>`. On OK, moves its value into the variable defined by
// `LHS`, otherwise returns the error status from the current function. If there
// is an error, `LHS` is not evaluated; thus any side effects that `LHS` may
// have only occur in the success case.
//
// Interface:
//
//   INK_ASSIGN_OR_RETURN(lhs, rexpr)
//
// WARNING: expands into multiple statements; it cannot be used in a single
// statement (e.g. as the body of an if statement without {})!
//
// Example: Declaring and initializing a new variable (ValueType can be anything
//          that can be initialized with assignment, including references):
//   INK_ASSIGN_OR_RETURN(ValueType value, MaybeGetValue(arg));
//
// Example: Assigning to an existing variable:
//   ValueType value;
//   INK_ASSIGN_OR_RETURN(value, MaybeGetValue(arg));
//
// Example: Assigning to an expression with side effects:
//   MyProto data;
//   INK_ASSIGN_OR_RETURN(*data.mutable_str(), MaybeGetValue(arg));
//   // No field "str" is added on error.
//
// Example: Assigning to a std::unique_ptr.
//   INK_ASSIGN_OR_RETURN(std::unique_ptr<T> ptr, MaybeGetPtr(arg));
#define INK_ASSIGN_OR_RETURN(LHS, STATUS_OR_EXPRESSION)                      \
  INK_ASSIGN_OR_RETURN_IMPL_(INK_MACROS_IMPL_CONCAT_(_status_or_, __LINE__), \
                             LHS, STATUS_OR_EXPRESSION)

#define INK_ASSIGN_OR_RETURN_IMPL_(statusor_variable, LHS, \
                                   STATUS_OR_EXPRESSION)   \
  auto statusor_variable = (STATUS_OR_EXPRESSION);         \
  if (ABSL_PREDICT_FALSE(!statusor_variable.ok())) {       \
    return statusor_variable.status();                     \
  }                                                        \
  LHS = std::move(statusor_variable).ValueOrDie();

// Internal helper for concatenating macro values.
#define INK_MACROS_IMPL_CONCAT_INNER_(x, y) x##y
#define INK_MACROS_IMPL_CONCAT_(x, y) INK_MACROS_IMPL_CONCAT_INNER_(x, y)

}  // namespace ink

#endif  // INK_ENGINE_PUBLIC_TYPES_STATUS_OR_H_
