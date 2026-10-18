#ifndef MYMONEY_STORAGE_SQLITE_STATEMENT_H_
#define MYMONEY_STORAGE_SQLITE_STATEMENT_H_

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "core/error.h"

struct sqlite3_stmt;  // defined in sqlite3.h, which only the wrapper's .cpp
                      // files see

namespace mymoney::storage::sqlite {

namespace internal {
template <typename T>
inline constexpr bool kIsOptional = false;
template <typename T>
inline constexpr bool kIsOptional<std::optional<T>> = true;
}  // namespace internal

// A prepared SQL statement (ADR-0013). Created by Database::Prepare; move-only,
// finalized automatically. Parameters are numbered ?1, ?2, ... in the SQL.
class Statement {
 public:
  // Binds all parameters in order: Bind(a, b) sets ?1 = a and ?2 = b.
  // Supported: std::int64_t, int, bool, text (std::string, std::string_view,
  // const char*), std::nullopt (NULL) and std::optional<T> of those.
  template <typename... Args>
  [[nodiscard]] core::Status Bind(const Args&... args) {
    if constexpr (sizeof...(Args) == 0) {
      return {};  // a statement without parameters
    } else {
      core::Status status;
      int index = 0;
      // A fold expression over &&: binds one argument after another and stops
      // at the first error, because && does not evaluate its right side when
      // the left side is false.
      // A string literal argument decays to const char* here; that is what
      // the const char* overload of BindValue is for (bounded by its '\0').
      // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      static_cast<void>(
          ((status = BindValue(++index, args)).has_value() && ...));
      // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      return status;
    }
  }

  // Executes one step: true - a result row is available, false - done.
  [[nodiscard]] core::Result<bool> Step();

  // Calls `on_row(*this)` for every result row; `on_row` returns core::Status.
  // Stops at the first error - from SQLite or from `on_row`.
  template <typename OnRow>
  [[nodiscard]] core::Status ForEachRow(OnRow on_row) {
    while (true) {
      core::Result<bool> row = Step();
      if (!row) {
        return std::unexpected(std::move(row).error());
      }
      if (!*row) {
        return {};
      }
      if (core::Status status = on_row(*this); !status) {
        return status;
      }
    }
  }

  // Makes the statement ready to be executed again (bindings are kept).
  [[nodiscard]] core::Status Reset();

  // Reads column `index` (0-based) of the current row.
  // Supported: std::int64_t, bool, std::string, std::optional<T> of those.
  template <typename T>
  [[nodiscard]] T Column(int index) const {
    if constexpr (std::is_same_v<T, std::int64_t>) {
      return ColumnInt64(index);
    } else if constexpr (std::is_same_v<T, bool>) {
      return ColumnInt64(index) != 0;
    } else if constexpr (std::is_same_v<T, std::string>) {
      return ColumnText(index);
    } else if constexpr (internal::kIsOptional<T>) {
      if (IsNull(index)) {
        return std::nullopt;
      }
      return Column<typename T::value_type>(index);
    } else {
      static_assert(false, "unsupported column type");
    }
  }

  [[nodiscard]] bool IsNull(int index) const;

 private:
  friend class Database;

  struct Finalizer {
    void operator()(sqlite3_stmt* statement) const noexcept;
  };

  explicit Statement(sqlite3_stmt* statement) : statement_(statement) {}

  [[nodiscard]] core::Status BindValue(int index, std::int64_t value);
  [[nodiscard]] core::Status BindValue(int index, int value) {
    return BindValue(index, std::int64_t{value});
  }
  [[nodiscard]] core::Status BindValue(int index, bool value) {
    return BindValue(index, std::int64_t{value ? 1 : 0});
  }
  [[nodiscard]] core::Status BindValue(int index, std::string_view value);
  // Without this overload a string literal would choose the bool overload:
  // const char* -> bool is a standard conversion and beats -> string_view.
  [[nodiscard]] core::Status BindValue(int index, const char* value) {
    return BindValue(index, std::string_view(value));
  }
  [[nodiscard]] core::Status BindValue(int index, const std::string& value) {
    return BindValue(index, std::string_view(value));
  }
  [[nodiscard]] core::Status BindValue(int index, std::nullopt_t /*null*/);
  template <typename T>
  [[nodiscard]] core::Status BindValue(int index,
                                       const std::optional<T>& value) {
    return value.has_value() ? BindValue(index, *value)
                             : BindValue(index, std::nullopt);
  }

  [[nodiscard]] std::int64_t ColumnInt64(int index) const;
  [[nodiscard]] std::string ColumnText(int index) const;

  // Rule of zero: unique_ptr makes Statement move-only and finalizes it.
  std::unique_ptr<sqlite3_stmt, Finalizer> statement_;
};

}  // namespace mymoney::storage::sqlite

#endif  // MYMONEY_STORAGE_SQLITE_STATEMENT_H_
