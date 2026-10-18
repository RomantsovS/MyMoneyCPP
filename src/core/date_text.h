#ifndef MYMONEY_CORE_DATE_TEXT_H_
#define MYMONEY_CORE_DATE_TEXT_H_

// Calendar dates as ISO 8601 text, "2026-10-18" (ADR-0014). The same format is
// used for user input, output and the database (ADR-0013).

#include <chrono>
#include <string>
#include <string_view>

#include "core/error.h"

namespace mymoney::core {

// Precondition: date.ok().
[[nodiscard]] std::string FormatDate(std::chrono::year_month_day date);

// Accepts exactly "YYYY-MM-DD" with a real calendar date; kInvalidArgument
// otherwise ("2026-02-30", "2026-1-5", "18.10.2026").
[[nodiscard]] Result<std::chrono::year_month_day> ParseDate(
    std::string_view text);

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_DATE_TEXT_H_
