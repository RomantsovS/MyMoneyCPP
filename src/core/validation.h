#ifndef MYMONEY_CORE_VALIDATION_H_
#define MYMONEY_CORE_VALIDATION_H_

#include <expected>
#include <format>
#include <string_view>

#include "core/error.h"

namespace mymoney::core {

// Checks a user-entered name: it must contain more than whitespace.
// `what` names the object in the error message, e.g. "account".
[[nodiscard]] Status ValidateName(std::string_view name, std::string_view what);

// For unique names (ADR-0014): `lookup` is the result of looking up an
// existing object by the new name. Success if nothing was found,
// kRuleViolation if something was; any other error is passed on.
template <typename T>
[[nodiscard]] Status CheckNameIsFree(const Result<T>& lookup,
                                     std::string_view what,
                                     std::string_view name) {
  if (lookup.has_value()) {
    return MakeError(ErrorCode::kRuleViolation,
                     std::format("{} '{}' already exists", what, name));
  }
  if (lookup.error().code == ErrorCode::kNotFound) {
    return {};
  }
  return std::unexpected(lookup.error());
}

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_VALIDATION_H_
