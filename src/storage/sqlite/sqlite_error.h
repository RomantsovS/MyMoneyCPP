#ifndef MYMONEY_STORAGE_SQLITE_SQLITE_ERROR_H_
#define MYMONEY_STORAGE_SQLITE_SQLITE_ERROR_H_

// Internal to the wrapper: used only by the .cpp files in storage/sqlite/.

#include <expected>
#include <string_view>

#include "core/error.h"

struct sqlite3;

namespace mymoney::storage::sqlite::internal {

// A kStorage error for a failed SQLite call: `action` says what we tried, the
// rest comes from SQLite (its message for `db`, or the text for `code`).
std::unexpected<core::Error> SqliteError(sqlite3* db, int code,
                                         std::string_view action);

}  // namespace mymoney::storage::sqlite::internal

#endif  // MYMONEY_STORAGE_SQLITE_SQLITE_ERROR_H_
