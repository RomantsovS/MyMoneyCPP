#include "storage/sqlite_account_repository.h"

#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "core/account.h"
#include "core/check.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/strong_id.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"

namespace mymoney::storage {
namespace {

constexpr std::string_view kSelectAccount =
    "SELECT id, name, currency, archived FROM account";

// Reads a row (id, name, currency, archived).
core::Result<core::Account> ReadAccount(const sqlite::Statement& row) {
  return core::CurrencyCode::Parse(row.Column<std::string>(2))
      .transform([&](core::CurrencyCode currency) {
        return core::Account{.id = core::AccountId(row.Column<std::int64_t>(0)),
                             .name = row.Column<std::string>(1),
                             .currency = currency,
                             .archived = row.Column<bool>(3)};
      });
}

std::unexpected<core::Error> NotFound(core::AccountId id) {
  return core::MakeError(core::ErrorCode::kNotFound,
                         std::format("no account with id {}", id.value()));
}

}  // namespace

SqliteAccountRepository::SqliteAccountRepository(sqlite::Database* db)
    : db_(db) {
  core::Check(db_ != nullptr, "SqliteAccountRepository needs a Database");
}

core::Result<core::Account> SqliteAccountRepository::Add(
    const core::NewAccount& account) {
  return db_
      ->Run("INSERT INTO account (name, currency) VALUES (?1, ?2)",
            account.name, account.currency.code())
      .transform([&] {
        return core::Account{.id = core::AccountId(db_->LastInsertId()),
                             .name = account.name,
                             .currency = account.currency,
                             .archived = false};
      });
}

core::Result<core::Account> SqliteAccountRepository::Get(core::AccountId id) {
  core::Result<sqlite::Statement> select =
      db_->Prepare(std::format("{} WHERE id = ?1", kSelectAccount), id.value());
  if (!select) {
    return std::unexpected(std::move(select).error());
  }
  return select->Step().and_then(
      [&](bool has_row) -> core::Result<core::Account> {
        if (!has_row) {
          return NotFound(id);
        }
        return ReadAccount(*select);
      });
}

core::Result<std::vector<core::Account>> SqliteAccountRepository::List() {
  std::vector<core::Account> accounts;
  return db_->Prepare(std::format("{} ORDER BY id", kSelectAccount))
      .and_then([&](sqlite::Statement select) {
        return select.ForEachRow([&](const sqlite::Statement& row) {
          return ReadAccount(row).transform([&](core::Account account) {
            accounts.push_back(std::move(account));
          });
        });
      })
      .transform([&] { return std::move(accounts); });
}

core::Status SqliteAccountRepository::SetArchived(core::AccountId id,
                                                  bool archived) {
  return db_
      ->Run("UPDATE account SET archived = ?2 WHERE id = ?1", id.value(),
            archived)
      .and_then([&]() -> core::Status {
        if (db_->ChangedRows() == 0) {
          return NotFound(id);
        }
        return {};
      });
}

}  // namespace mymoney::storage
