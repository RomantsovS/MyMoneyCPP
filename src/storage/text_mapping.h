#ifndef MYMONEY_STORAGE_TEXT_MAPPING_H_
#define MYMONEY_STORAGE_TEXT_MAPPING_H_

// How domain values are written to and read from TEXT columns (ADR-0013).
// The mapping is explicit: enum values never depend on their C++ numbering.

#include <chrono>
#include <string>
#include <string_view>

#include "core/category.h"
#include "core/error.h"
#include "core/operation.h"

namespace mymoney::storage {

// "2026-10-18" (ISO 8601) - sorts and compares correctly as plain text.
[[nodiscard]] std::string DateToText(std::chrono::year_month_day date);
[[nodiscard]] core::Result<std::chrono::year_month_day> DateFromText(
    std::string_view text);

[[nodiscard]] std::string_view CategoryKindToText(core::CategoryKind kind);
[[nodiscard]] core::Result<core::CategoryKind> CategoryKindFromText(
    std::string_view text);

[[nodiscard]] std::string_view OperationTypeToText(core::OperationType type);
[[nodiscard]] core::Result<core::OperationType> OperationTypeFromText(
    std::string_view text);

}  // namespace mymoney::storage

#endif  // MYMONEY_STORAGE_TEXT_MAPPING_H_
