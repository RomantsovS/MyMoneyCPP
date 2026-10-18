#include "storage/text_mapping.h"

#include <chrono>
#include <expected>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

#include "core/category.h"
#include "core/check.h"
#include "core/date_text.h"
#include "core/error.h"
#include "core/operation.h"

namespace mymoney::storage {
namespace {

std::unexpected<core::Error> BadText(std::string_view what,
                                     std::string_view text) {
  return core::MakeError(
      core::ErrorCode::kStorage,
      std::format("invalid {} in the database: '{}'", what, text));
}

}  // namespace

// The date format is defined once, in core (ADR-0014).
std::string DateToText(std::chrono::year_month_day date) {
  return core::FormatDate(date);
}

core::Result<std::chrono::year_month_day> DateFromText(std::string_view text) {
  // A bad date read from the database is a storage problem, not bad input.
  return core::ParseDate(text).or_else(
      [&](const core::Error& /*error*/)
          -> core::Result<std::chrono::year_month_day> {
        return BadText("date", text);
      });
}

std::string_view CategoryKindToText(core::CategoryKind kind) {
  switch (kind) {
    case core::CategoryKind::kExpense:
      return "expense";
    case core::CategoryKind::kIncome:
      return "income";
  }
  core::CheckFailed("unknown CategoryKind", std::source_location::current());
}

core::Result<core::CategoryKind> CategoryKindFromText(std::string_view text) {
  if (text == "expense") {
    return core::CategoryKind::kExpense;
  }
  if (text == "income") {
    return core::CategoryKind::kIncome;
  }
  return BadText("category kind", text);
}

std::string_view OperationTypeToText(core::OperationType type) {
  switch (type) {
    case core::OperationType::kExpense:
      return "expense";
    case core::OperationType::kIncome:
      return "income";
    case core::OperationType::kTransfer:
      return "transfer";
  }
  core::CheckFailed("unknown OperationType", std::source_location::current());
}

core::Result<core::OperationType> OperationTypeFromText(std::string_view text) {
  if (text == "expense") {
    return core::OperationType::kExpense;
  }
  if (text == "income") {
    return core::OperationType::kIncome;
  }
  if (text == "transfer") {
    return core::OperationType::kTransfer;
  }
  return BadText("operation type", text);
}

}  // namespace mymoney::storage
