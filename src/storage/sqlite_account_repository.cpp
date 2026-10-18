#include "storage/sqlite_account_repository.h"

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "core/account.h"
#include "core/check.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/strong_id.h"
#include "storage/query_helpers.h"
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

std::string NotFoundMessage(core::AccountId id) {
  return std::format("no account with id {}", id.value());
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
  return internal::QueryOne(*db_,
                            std::format("{} WHERE id = ?1", kSelectAccount),
                            NotFoundMessage(id), ReadAccount, id.value());
}

core::Result<core::Account> SqliteAccountRepository::GetByName(
    std::string_view name) {
  return internal::QueryOne(
      *db_, std::format("{} WHERE name = ?1", kSelectAccount),
      std::format("no account named '{}'", name), ReadAccount, name);
}

core::Result<std::vector<core::Account>> SqliteAccountRepository::List() {
  return internal::QueryAll<core::Account>(
      *db_, std::format("{} ORDER BY id", kSelectAccount), ReadAccount);
}

core::Status SqliteAccountRepository::SetArchived(core::AccountId id,
                                                  bool archived) {
  return db_
      ->Run("UPDATE account SET archived = ?2 WHERE id = ?1", id.value(),
            archived)
      .and_then([&]() -> core::Status {
        if (db_->ChangedRows() == 0) {
          return core::MakeError(core::ErrorCode::kNotFound,
                                 NotFoundMessage(id));
        }
        return {};
      });
}

}  // namespace mymoney::storage
