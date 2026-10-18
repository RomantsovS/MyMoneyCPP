#ifndef MYMONEY_STORAGE_SQLITE_DATABASE_H_
#define MYMONEY_STORAGE_SQLITE_DATABASE_H_

#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "core/error.h"
#include "storage/sqlite/statement.h"

struct sqlite3;  // defined in sqlite3.h, which only the wrapper's .cpp files
                 // see

namespace mymoney::storage::sqlite {

// A connection to one SQLite database (ADR-0013). Move-only, closed
// automatically. Use from one thread at a time.
class Database {
 public:
  // Opens or creates the database file; ":memory:" gives a temporary
  // in-memory database (used by tests).
  [[nodiscard]] static core::Result<Database> Open(const std::string& path);

  // Runs one or more SQL statements that return no rows we need
  // (CREATE TABLE, PRAGMA, BEGIN, ...).
  [[nodiscard]] core::Status Execute(const std::string& sql);

  // Compiles exactly one SQL statement and binds `args` to ?1, ?2, ...
  template <typename... Args>
  [[nodiscard]] core::Result<Statement> Prepare(std::string_view sql,
                                                const Args&... args) {
    core::Result<Statement> statement = PrepareStatement(sql);
    if (statement) {
      if (core::Status status = statement->Bind(args...); !status) {
        return std::unexpected(std::move(status).error());
      }
    }
    return statement;
  }

  // Runs one statement that returns no rows (INSERT, UPDATE, DELETE).
  template <typename... Args>
  [[nodiscard]] core::Status Run(std::string_view sql, const Args&... args) {
    return Prepare(sql, args...).and_then([](Statement statement) {
      return statement.Step().transform([](bool /*has_row*/) {});
    });
  }

  // The id (rowid) of the last row inserted through this connection.
  [[nodiscard]] std::int64_t LastInsertId() const;

  // How many rows the last INSERT, UPDATE or DELETE changed.
  [[nodiscard]] std::int64_t ChangedRows() const;

 private:
  struct Closer {
    void operator()(sqlite3* db) const noexcept;
  };

  explicit Database(sqlite3* db) : db_(db) {}

  [[nodiscard]] core::Result<Statement> PrepareStatement(std::string_view sql);

  // Rule of zero: unique_ptr makes Database move-only and closes it.
  std::unique_ptr<sqlite3, Closer> db_;
};

}  // namespace mymoney::storage::sqlite

#endif  // MYMONEY_STORAGE_SQLITE_DATABASE_H_
