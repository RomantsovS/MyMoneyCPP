#ifndef MYMONEY_CORE_ERROR_H_
#define MYMONEY_CORE_ERROR_H_

#include <cstdint>
#include <expected>
#include <string>
#include <utility>

namespace mymoney::core {

// What kind of failure an Error describes (ADR-0012). The UI maps codes to
// user-facing texts; Error::message is for developers and logs.
enum class ErrorCode : std::uint8_t {
  kInvalidArgument,  // malformed input: empty name, invalid date, amount <= 0
  kNotFound,         // a referenced object does not exist
  kRuleViolation,    // valid input that breaks a business rule
  kStorage,          // the storage layer failed (M2)
};

struct Error {
  ErrorCode code;
  std::string message;

  friend bool operator==(const Error&, const Error&) = default;
};

// Functions that can fail return a value or an Error - never throw (ADR-0008).
template <typename T>
using Result = std::expected<T, Error>;
using Status = std::expected<void, Error>;

// Shortcut for returning an error: `return MakeError(ErrorCode::kNotFound,
// "...");`
inline std::unexpected<Error> MakeError(ErrorCode code, std::string message) {
  return std::unexpected(Error{.code = code, .message = std::move(message)});
}

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_ERROR_H_
