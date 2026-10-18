#include "storage/sqlite_currency_repository.h"

#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <vector>

#include "core/check.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "storage/query_helpers.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"

namespace mymoney::storage {
namespace {

// Reads a row (code, minor_digits) and validates it like new input (ADR-0013).
core::Result<core::Currency> ReadCurrency(const sqlite::Statement& row) {
  const auto minor_digits = row.Column<std::int64_t>(1);
  if (minor_digits < 0 || minor_digits > std::numeric_limits<int>::max()) {
    return core::MakeError(core::ErrorCode::kStorage,
                           "invalid minor_digits in the database");
  }
  return core::CurrencyCode::Parse(row.Column<std::string>(0))
      .and_then([&](core::CurrencyCode code) {
        return core::Currency::Create(code, static_cast<int>(minor_digits));
      });
}

}  // namespace

SqliteCurrencyRepository::SqliteCurrencyRepository(sqlite::Database* db)
    : db_(db) {
  core::Check(db_ != nullptr, "SqliteCurrencyRepository needs a Database");
}

core::Status SqliteCurrencyRepository::Add(const core::Currency& currency) {
  // Checked explicitly so the caller gets kRuleViolation, not a raw
  // "UNIQUE constraint failed" storage error.
  if (Get(currency.code()).has_value()) {
    return core::MakeError(
        core::ErrorCode::kRuleViolation,
        std::format("currency {} already exists", currency.code().code()));
  }
  return db_->Run("INSERT INTO currency (code, minor_digits) VALUES (?1, ?2)",
                  currency.code().code(), currency.minor_digits());
}

core::Result<core::Currency> SqliteCurrencyRepository::Get(
    core::CurrencyCode code) {
  return internal::QueryOne(
      *db_, "SELECT code, minor_digits FROM currency WHERE code = ?1",
      std::format("unknown currency {}", code.code()), ReadCurrency,
      code.code());
}

core::Result<std::vector<core::Currency>> SqliteCurrencyRepository::List() {
  return internal::QueryAll<core::Currency>(
      *db_, "SELECT code, minor_digits FROM currency ORDER BY code",
      ReadCurrency);
}

}  // namespace mymoney::storage
