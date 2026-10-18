#include "core/amount_text.h"

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "core/check.h"
#include "core/currency.h"
#include "core/error.h"
#include "core/money.h"

namespace mymoney::core {
namespace {

void CheckMinorDigits(int minor_digits) {
  Check(minor_digits >= 0 && minor_digits <= Currency::kMaxMinorDigits,
        "minor_digits out of range");
}

std::int64_t PowerOfTen(int exponent) {
  std::int64_t result = 1;
  for (int i = 0; i < exponent; ++i) {
    result *= 10;
  }
  return result;
}

bool IsDigits(std::string_view text) {
  return std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; });
}

}  // namespace

Result<std::int64_t> ParseAmount(std::string_view text, int minor_digits) {
  CheckMinorDigits(minor_digits);
  const auto invalid = [&](std::string_view reason) {
    return MakeError(ErrorCode::kInvalidArgument,
                     std::format("invalid amount '{}': {}", text, reason));
  };

  const std::size_t separator = text.find_first_of(".,");
  const std::string_view whole = text.substr(0, separator);
  const std::string_view fraction = separator == std::string_view::npos
                                        ? std::string_view{}
                                        : text.substr(separator + 1);

  if (whole.empty() || !IsDigits(whole) || !IsDigits(fraction) ||
      (separator != std::string_view::npos && fraction.empty())) {
    return invalid("expected digits with an optional decimal part, e.g. 12.50");
  }
  if (std::cmp_greater(fraction.size(), minor_digits)) {
    return invalid(
        minor_digits == 0
            ? std::string("this currency has no decimals")
            : std::format("at most {} decimal places", minor_digits));
  }

  // whole * 10^minor_digits + fraction, padded: "12.5" with 2 digits -> 1250.
  // Each step is checked against the input limit, so nothing can overflow.
  const std::int64_t scale = PowerOfTen(minor_digits);
  const std::int64_t max_whole = Money::kMaxInputMinorUnits / scale;
  std::int64_t value = 0;
  for (const char c : whole) {
    value = (value * 10) + (c - '0');
    if (value > max_whole) {
      return invalid("too large");
    }
  }
  std::int64_t fraction_value = 0;
  for (const char c : fraction) {
    fraction_value = (fraction_value * 10) + (c - '0');
  }
  fraction_value *=
      PowerOfTen(minor_digits - static_cast<int>(fraction.size()));

  value = (value * scale) + fraction_value;
  if (value > Money::kMaxInputMinorUnits) {
    return invalid("too large");
  }
  return value;
}

std::string FormatAmount(std::int64_t minor_units, int minor_digits) {
  CheckMinorDigits(minor_digits);
  // The magnitude as unsigned, so that even INT64_MIN has a positive value.
  const std::uint64_t magnitude =
      minor_units < 0 ? 0U - static_cast<std::uint64_t>(minor_units)
                      : static_cast<std::uint64_t>(minor_units);
  const auto scale = static_cast<std::uint64_t>(PowerOfTen(minor_digits));
  const std::string_view sign = minor_units < 0 ? "-" : "";
  if (minor_digits == 0) {
    return std::format("{}{}", sign, magnitude);
  }
  return std::format("{}{}.{:0{}}", sign, magnitude / scale, magnitude % scale,
                     minor_digits);
}

}  // namespace mymoney::core
