#ifndef MYMONEY_CORE_AMOUNT_TEXT_H_
#define MYMONEY_CORE_AMOUNT_TEXT_H_

// Amounts as text in a fixed, locale-independent format (ADR-0014).
// Locale-aware display ("1 234,50 €") is the job of the Qt UI.

#include <cstdint>
#include <string>
#include <string_view>

#include "core/error.h"

namespace mymoney::core {

// Parses a positive amount entered by a user into minor units:
// ParseAmount("12.50", 2) == 1250. Accepts "." or "," as the decimal
// separator and at most `minor_digits` decimals. Rejects signs, thousands
// separators and amounts above Money::kMaxInputMinorUnits (kInvalidArgument).
// Precondition: 0 <= minor_digits <= Currency::kMaxMinorDigits.
[[nodiscard]] Result<std::int64_t> ParseAmount(std::string_view text,
                                               int minor_digits);

// Formats minor units with exactly `minor_digits` decimals:
// FormatAmount(-123450, 2) == "-1234.50", FormatAmount(500, 0) == "500".
// Precondition: 0 <= minor_digits <= Currency::kMaxMinorDigits.
[[nodiscard]] std::string FormatAmount(std::int64_t minor_units,
                                       int minor_digits);

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_AMOUNT_TEXT_H_
