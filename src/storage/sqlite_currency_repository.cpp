#include "storage/sqlite_currency_repository.h"

#include <cstdint>
#include <expected>
#include <format>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "core/check.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
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
  core::Result<sqlite::Statement> select = db_->Prepare(
      "SELECT code, minor_digits FROM currency WHERE code = ?1", code.code());
  if (!select) {
    return std::unexpected(std::move(select).error());
  }
  return select->Step().and_then(
      [&](bool has_row) -> core::Result<core::Currency> {
        if (!has_row) {
          return core::MakeError(
              core::ErrorCode::kNotFound,
              std::format("unknown currency {}", code.code()));
        }
        return ReadCurrency(*select);
      });
}

core::Result<std::vector<core::Currency>> SqliteCurrencyRepository::List() {
  std::vector<core::Currency> currencies;
  return db_->Prepare("SELECT code, minor_digits FROM currency ORDER BY code")
      .and_then([&](sqlite::Statement select) {
        return select.ForEachRow([&](const sqlite::Statement& row) {
          return ReadCurrency(row).transform(
              [&](core::Currency currency) { currencies.push_back(currency); });
        });
      })
      .transform([&] { return std::move(currencies); });
}

}  // namespace mymoney::storage
