#include "storage/sqlite/statement.h"

#include <sqlite3.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "core/check.h"
#include "core/error.h"
#include "storage/sqlite/sqlite_error.h"

namespace mymoney::storage::sqlite {
namespace {

sqlite3* ConnectionOf(sqlite3_stmt* statement) {
  return sqlite3_db_handle(statement);
}

}  // namespace

void Statement::Finalizer::operator()(sqlite3_stmt* statement) const noexcept {
  sqlite3_finalize(statement);
}

core::Status Statement::BindValue(int index, std::int64_t value) {
  const int code = sqlite3_bind_int64(statement_.get(), index, value);
  if (code != SQLITE_OK) {
    return internal::SqliteError(ConnectionOf(statement_.get()), code,
                                 "bind integer");
  }
  return {};
}

core::Status Statement::BindValue(int index, std::string_view value) {
  core::Check(
      value.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()),
      "text too long for SQLite");
  // SQLITE_TRANSIENT: SQLite copies the text, so the caller's string may be a
  // temporary that is gone before the statement runs.
  const int code =
      sqlite3_bind_text(statement_.get(), index, value.data(),
                        static_cast<int>(value.size()), SQLITE_TRANSIENT);
  if (code != SQLITE_OK) {
    return internal::SqliteError(ConnectionOf(statement_.get()), code,
                                 "bind text");
  }
  return {};
}

core::Status Statement::BindValue(int index, std::nullopt_t /*null*/) {
  const int code = sqlite3_bind_null(statement_.get(), index);
  if (code != SQLITE_OK) {
    return internal::SqliteError(ConnectionOf(statement_.get()), code,
                                 "bind NULL");
  }
  return {};
}

core::Result<bool> Statement::Step() {
  const int code = sqlite3_step(statement_.get());
  if (code == SQLITE_ROW) {
    return true;
  }
  if (code == SQLITE_DONE) {
    return false;
  }
  return internal::SqliteError(ConnectionOf(statement_.get()), code,
                               "execute statement");
}

core::Status Statement::Reset() {
  const int code = sqlite3_reset(statement_.get());
  if (code != SQLITE_OK) {
    return internal::SqliteError(ConnectionOf(statement_.get()), code,
                                 "reset statement");
  }
  return {};
}

bool Statement::IsNull(int index) const {
  return sqlite3_column_type(statement_.get(), index) == SQLITE_NULL;
}

std::int64_t Statement::ColumnInt64(int index) const {
  return sqlite3_column_int64(statement_.get(), index);
}

std::string Statement::ColumnText(int index) const {
  // sqlite3_column_text returns unsigned char; the bytes are UTF-8 text.
  const void* text = sqlite3_column_text(statement_.get(), index);
  const int size = sqlite3_column_bytes(statement_.get(), index);
  if (text == nullptr) {
    return {};
  }
  return {static_cast<const char*>(text), static_cast<std::size_t>(size)};
}

}  // namespace mymoney::storage::sqlite
