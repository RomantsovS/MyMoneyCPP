#include "storage/migrations.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <limits>
#include <span>
#include <string>
#include <utility>

#include "core/check.h"
#include "core/error.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"
#include "storage/sqlite/transaction.h"

namespace mymoney::storage {
namespace {

core::Status ApplyOne(sqlite::Database& db, const MigrationScript& script) {
  core::Result<sqlite::Transaction> transaction =
      sqlite::Transaction::Begin(db);
  if (!transaction) {
    return std::unexpected(std::move(transaction).error());
  }
  // Script and version update in one transaction: either both or neither.
  core::Status status = db.Execute(std::string(script.sql))
                            .and_then([&] {
                              return db.Execute(std::format(
                                  "PRAGMA user_version = {}", script.version));
                            })
                            .and_then([&] { return transaction->Commit(); });
  if (!status) {
    return core::MakeError(core::ErrorCode::kStorage,
                           std::format("migration {} failed: {}", script.name,
                                       status.error().message));
  }
  return {};
}

}  // namespace

core::Result<int> SchemaVersion(sqlite::Database& db) {
  core::Result<sqlite::Statement> statement = db.Prepare("PRAGMA user_version");
  if (!statement) {
    return std::unexpected(std::move(statement).error());
  }
  const core::Result<bool> row = statement->Step();
  if (!row) {
    return std::unexpected(row.error());
  }
  const auto version = statement->Column<std::int64_t>(0);
  core::Check(version >= 0 && version <= std::numeric_limits<int>::max(),
              "PRAGMA user_version out of range");
  return static_cast<int>(version);
}

core::Status ApplyMigrations(sqlite::Database& db,
                             std::span<const MigrationScript> scripts) {
  int expected_version = 1;
  for (const MigrationScript& script : scripts) {
    core::Check(script.version == expected_version,
                "migration scripts must have versions 1, 2, 3, ...");
    ++expected_version;
  }
  const int newest = static_cast<int>(scripts.size());

  const core::Result<int> current = SchemaVersion(db);
  if (!current) {
    return std::unexpected(current.error());
  }
  if (*current > newest) {
    return core::MakeError(
        core::ErrorCode::kStorage,
        std::format("the database has schema version {}, but this app knows "
                    "only up to {}; please update the app",
                    *current, newest));
  }

  for (const MigrationScript& script :
       scripts.subspan(static_cast<std::size_t>(*current))) {
    if (core::Status status = ApplyOne(db, script); !status) {
      return status;
    }
  }
  return {};
}

}  // namespace mymoney::storage
