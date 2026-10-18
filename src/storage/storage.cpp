#include "storage/storage.h"

#include <expected>
#include <string>
#include <utility>

#include "core/error.h"
#include "storage/migrations.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage {

core::Result<sqlite::Database> OpenStorage(const std::string& path) {
  core::Result<sqlite::Database> db = sqlite::Database::Open(path);
  if (!db) {
    return db;
  }
  // foreign_keys: also compiled in as the default; set explicitly so the
  // guarantee does not depend on how SQLite was built.
  // journal_mode DELETE: the database is one file whenever no transaction
  // runs, so a backup is a plain file copy.
  core::Status status =
      db->Execute("PRAGMA foreign_keys = ON; PRAGMA journal_mode = DELETE;")
          .and_then(
              [&] { return ApplyMigrations(*db, EmbeddedMigrationScripts()); });
  if (!status) {
    return std::unexpected(std::move(status).error());
  }
  return db;
}

}  // namespace mymoney::storage
