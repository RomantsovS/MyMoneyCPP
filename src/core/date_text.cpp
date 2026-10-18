#include "core/date_text.h"

#include <chrono>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "core/check.h"
#include "core/error.h"

namespace mymoney::core {
namespace {

// Parses a non-negative decimal number made of digits only ("0042" -> 42).
std::optional<int> ParseDigits(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }
  int value = 0;
  for (const char c : text) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    value = (value * 10) + (c - '0');
  }
  return value;
}

}  // namespace

std::string FormatDate(std::chrono::year_month_day date) {
  Check(date.ok(), "FormatDate needs a valid date");
  return std::format("{:04}-{:02}-{:02}", static_cast<int>(date.year()),
                     static_cast<unsigned>(date.month()),
                     static_cast<unsigned>(date.day()));
}

Result<std::chrono::year_month_day> ParseDate(std::string_view text) {
  const auto invalid = [&] {
    return MakeError(
        ErrorCode::kInvalidArgument,
        std::format("invalid date '{}': expected YYYY-MM-DD", text));
  };
  if (text.size() != 10 || text.substr(4, 1) != "-" ||
      text.substr(7, 1) != "-") {
    return invalid();
  }
  const std::optional<int> year = ParseDigits(text.substr(0, 4));
  const std::optional<int> month = ParseDigits(text.substr(5, 2));
  const std::optional<int> day = ParseDigits(text.substr(8, 2));
  if (!year || !month || !day) {
    return invalid();
  }
  const std::chrono::year_month_day date{
      std::chrono::year{*year},
      std::chrono::month{static_cast<unsigned>(*month)},
      std::chrono::day{static_cast<unsigned>(*day)}};
  if (!date.ok()) {  // e.g. "2026-02-30"
    return invalid();
  }
  return date;
}

}  // namespace mymoney::core
