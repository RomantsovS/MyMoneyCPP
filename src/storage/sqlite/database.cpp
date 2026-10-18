#include "storage/sqlite/database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <string_view>

#include "core/check.h"
#include "core/error.h"
#include "storage/sqlite/sqlite_error.h"
#include "storage/sqlite/statement.h"

namespace mymoney::storage::sqlite {
namespace {

bool IsBlankOrSemicolons(std::string_view text) {
  return std::ranges::all_of(text, [](char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ';';
  });
}

}  // namespace

void Database::Closer::operator()(sqlite3* db) const noexcept {
  // _v2: if statements are still open, SQLite closes the connection as soon
  // as the last of them is finalized instead of failing.
  sqlite3_close_v2(db);
}

core::Result<Database> Database::Open(const std::string& path) {
  sqlite3* raw = nullptr;
  const int code = sqlite3_open_v2(
      path.c_str(), &raw,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_EXRESCODE,
      nullptr);
  // SQLite allocates the handle even if opening fails; owning it right away
  // guarantees it is closed on every path.
  Database db(raw);
  if (code != SQLITE_OK) {
    return internal::SqliteError(raw, code,
                                 std::format("open database '{}'", path));
  }
  return db;
}

core::Status Database::Execute(const std::string& sql) {
  const int code =
      sqlite3_exec(db_.get(), sql.c_str(), nullptr, nullptr, nullptr);
  if (code != SQLITE_OK) {
    return internal::SqliteError(db_.get(), code, "execute SQL");
  }
  return {};
}

core::Result<Statement> Database::PrepareStatement(std::string_view sql) {
  core::Check(
      sql.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()),
      "SQL too long");
  sqlite3_stmt* raw = nullptr;
  const int code = sqlite3_prepare_v3(
      db_.get(), sql.data(), static_cast<int>(sql.size()), 0, &raw, nullptr);
  Statement statement(raw);  // owns the handle on every path
  if (code != SQLITE_OK) {
    return internal::SqliteError(db_.get(), code,
                                 std::format("prepare '{}'", sql));
  }
  if (raw == nullptr) {
    return core::MakeError(core::ErrorCode::kStorage,
                           "prepare: the SQL contains no statement");
  }
  // sqlite3_sql() is the text of the compiled (first) statement. Anything
  // after it would be silently ignored, so it is an error.
  const std::string_view compiled = sqlite3_sql(raw);
  if (!IsBlankOrSemicolons(sql.substr(std::min(compiled.size(), sql.size())))) {
    return core::MakeError(
        core::ErrorCode::kStorage,
        std::format("prepare: more than one statement in '{}'", sql));
  }
  return statement;
}

std::int64_t Database::LastInsertId() const {
  return sqlite3_last_insert_rowid(db_.get());
}

std::int64_t Database::ChangedRows() const {
  return sqlite3_changes64(db_.get());
}

}  // namespace mymoney::storage::sqlite
