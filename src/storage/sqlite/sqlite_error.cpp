#include "storage/sqlite/sqlite_error.h"

#include <sqlite3.h>

#include <expected>
#include <format>
#include <string_view>

#include "core/error.h"

namespace mymoney::storage::sqlite::internal {

std::unexpected<core::Error> SqliteError(sqlite3* db, int code,
                                         std::string_view action) {
  // With a connection, sqlite3_errmsg() describes this concrete failure
  // (e.g. "no such table: x"); without one only the generic text is known.
  const char* detail =
      db != nullptr ? sqlite3_errmsg(db) : sqlite3_errstr(code);
  return core::MakeError(
      core::ErrorCode::kStorage,
      std::format("{}: {} (SQLite error {})", action, detail, code));
}

}  // namespace mymoney::storage::sqlite::internal
