#include "storage/text_mapping.h"

#include <chrono>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>

#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/operation.h"

namespace mymoney::storage {
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

std::unexpected<core::Error> BadText(std::string_view what,
                                     std::string_view text) {
  return core::MakeError(
      core::ErrorCode::kStorage,
      std::format("invalid {} in the database: '{}'", what, text));
}

}  // namespace

std::string DateToText(std::chrono::year_month_day date) {
  core::Check(date.ok(), "DateToText needs a valid date");
  return std::format("{:04}-{:02}-{:02}", static_cast<int>(date.year()),
                     static_cast<unsigned>(date.month()),
                     static_cast<unsigned>(date.day()));
}

core::Result<std::chrono::year_month_day> DateFromText(std::string_view text) {
  // Exactly "YYYY-MM-DD".
  if (text.size() != 10 || text.substr(4, 1) != "-" ||
      text.substr(7, 1) != "-") {
    return BadText("date", text);
  }
  const std::optional<int> year = ParseDigits(text.substr(0, 4));
  const std::optional<int> month = ParseDigits(text.substr(5, 2));
  const std::optional<int> day = ParseDigits(text.substr(8, 2));
  if (!year || !month || !day) {
    return BadText("date", text);
  }
  const std::chrono::year_month_day date{
      std::chrono::year{*year},
      std::chrono::month{static_cast<unsigned>(*month)},
      std::chrono::day{static_cast<unsigned>(*day)}};
  if (!date.ok()) {  // e.g. "2026-02-30"
    return BadText("date", text);
  }
  return date;
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
