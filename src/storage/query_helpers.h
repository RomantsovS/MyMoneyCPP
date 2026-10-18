#ifndef MYMONEY_STORAGE_QUERY_HELPERS_H_
#define MYMONEY_STORAGE_QUERY_HELPERS_H_

// Internal to the storage layer: the two query shapes every repository needs.

#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/error.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"

namespace mymoney::storage::internal {

// Runs a query that returns at most one row and reads that row with
// `read(row)`, which returns core::Result<T>. No row: kNotFound with
// `not_found` as the message.
template <typename Read, typename... Args>
auto QueryOne(sqlite::Database& db, std::string_view sql, std::string not_found,
              Read read, const Args&... args)
    -> std::invoke_result_t<Read, const sqlite::Statement&> {
  using ResultType = std::invoke_result_t<Read, const sqlite::Statement&>;
  core::Result<sqlite::Statement> select = db.Prepare(sql, args...);
  if (!select) {
    return std::unexpected(std::move(select).error());
  }
  return select->Step().and_then([&](bool has_row) -> ResultType {
    if (!has_row) {
      return core::MakeError(core::ErrorCode::kNotFound, std::move(not_found));
    }
    return read(*select);
  });
}

// Runs a query and reads every row with `read(row)` (returning
// core::Result<T>) into a vector; stops at the first error.
template <typename T, typename Read, typename... Args>
core::Result<std::vector<T>> QueryAll(sqlite::Database& db,
                                      std::string_view sql, Read read,
                                      const Args&... args) {
  std::vector<T> rows;
  return db.Prepare(sql, args...)
      .and_then([&](sqlite::Statement select) {
        return select.ForEachRow([&](const sqlite::Statement& row) {
          return read(row).transform(
              [&](T value) { rows.push_back(std::move(value)); });
        });
      })
      .transform([&] { return std::move(rows); });
}

}  // namespace mymoney::storage::internal

#endif  // MYMONEY_STORAGE_QUERY_HELPERS_H_
