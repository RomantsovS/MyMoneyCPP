#include "storage/sqlite_operation_repository.h"

#include <chrono>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/check.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/strong_id.h"
#include "storage/query_helpers.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"
#include "storage/sqlite/transaction.h"
#include "storage/text_mapping.h"

namespace mymoney::storage {
namespace {

// The stored parts of an operation, before they are validated again.
struct StoredOperation {
  std::chrono::year_month_day date;
  core::OperationType type;
  std::string comment;
  std::vector<core::Movement> movements;
};

std::unexpected<core::Error> Corrupt(core::OperationId id,
                                     std::string_view what) {
  return core::MakeError(
      core::ErrorCode::kStorage,
      std::format("operation {} in the database is invalid: {}", id.value(),
                  what));
}

// Reads a movement row (account_id, category_id, amount_minor, currency).
core::Result<core::Movement> ReadMovement(const sqlite::Statement& row) {
  const auto amount = row.Column<std::int64_t>(2);
  // Bounds first: the factories negate amounts, and negating INT64_MIN
  // would break a precondition of Money.
  if (amount < -core::Money::kMaxInputMinorUnits ||
      amount > core::Money::kMaxInputMinorUnits) {
    return core::MakeError(
        core::ErrorCode::kStorage,
        std::format("movement amount {} is out of range", amount));
  }
  return core::CurrencyCode::Parse(row.Column<std::string>(3))
      .transform([&](core::CurrencyCode currency) {
        const auto category_id = row.Column<std::optional<std::int64_t>>(1);
        return core::Movement{
            .account_id = core::AccountId(row.Column<std::int64_t>(0)),
            .category_id = category_id.transform(
                [](std::int64_t value) { return core::CategoryId(value); }),
            .amount = core::Money(amount, currency)};
      });
}

// Turns stored rows back into a draft through the M1 factories, so every
// shape rule of ADR-0006 is checked again.
core::Result<core::OperationDraft> Rebuild(core::OperationId id,
                                           StoredOperation stored) {
  const std::vector<core::Movement>& movements = stored.movements;
  switch (stored.type) {
    case core::OperationType::kExpense:
    case core::OperationType::kIncome: {
      if (movements.size() != 1 || !movements.front().category_id) {
        return Corrupt(id, "expected one movement with a category");
      }
      const core::Movement& movement = movements.front();
      // Stored signed (expense negative); the factories take the positive
      // amount and apply the sign themselves.
      const bool expense = stored.type == core::OperationType::kExpense;
      const core::Money amount = expense ? -movement.amount : movement.amount;
      return expense
                 ? core::OperationDraft::Expense(
                       stored.date, movement.account_id, *movement.category_id,
                       amount, std::move(stored.comment))
                 : core::OperationDraft::Income(
                       stored.date, movement.account_id, *movement.category_id,
                       amount, std::move(stored.comment));
    }
    case core::OperationType::kTransfer: {
      if (movements.size() != 2) {
        return Corrupt(id, "a transfer needs two movements");
      }
      // Inserted in this order by Add(): first out, then in.
      const core::Movement& out = movements.front();
      const core::Movement& in = movements.back();
      return core::OperationDraft::Transfer(
          stored.date, out.account_id, -out.amount, in.account_id, in.amount,
          std::move(stored.comment));
    }
  }
  return Corrupt(id, "unknown operation type");
}

}  // namespace

SqliteOperationRepository::SqliteOperationRepository(sqlite::Database* db)
    : db_(db) {
  core::Check(db_ != nullptr, "SqliteOperationRepository needs a Database");
}

core::Result<core::Operation> SqliteOperationRepository::Add(
    const core::OperationDraft& draft) {
  // A scope guard: any early return below rolls everything back.
  core::Result<sqlite::Transaction> transaction =
      sqlite::Transaction::Begin(*db_);
  if (!transaction) {
    return std::unexpected(std::move(transaction).error());
  }
  if (core::Status status = db_->Run(
          "INSERT INTO operation (date, type, comment) VALUES (?1, ?2, ?3)",
          DateToText(draft.date()), OperationTypeToText(draft.type()),
          draft.comment());
      !status) {
    return std::unexpected(std::move(status).error());
  }
  const core::OperationId id(db_->LastInsertId());

  for (const core::Movement& movement : draft.movements()) {
    const std::optional<std::int64_t> category_id =
        movement.category_id.transform(
            [](core::CategoryId category) { return category.value(); });
    if (core::Status status = db_->Run(
            "INSERT INTO movement (operation_id, account_id, category_id, "
            "amount_minor) VALUES (?1, ?2, ?3, ?4)",
            id.value(), movement.account_id.value(), category_id,
            movement.amount.minor_units());
        !status) {
      return std::unexpected(std::move(status).error());
    }
  }

  return transaction->Commit().transform(
      [&] { return core::Operation{.id = id, .draft = draft}; });
}

core::Result<core::Operation> SqliteOperationRepository::Get(
    core::OperationId id) {
  core::Result<sqlite::Statement> select_operation = db_->Prepare(
      "SELECT date, type, comment FROM operation WHERE id = ?1", id.value());
  if (!select_operation) {
    return std::unexpected(std::move(select_operation).error());
  }
  const core::Result<bool> found = select_operation->Step();
  if (!found) {
    return std::unexpected(found.error());
  }
  if (!*found) {
    return core::MakeError(core::ErrorCode::kNotFound,
                           std::format("no operation with id {}", id.value()));
  }
  const core::Result<std::chrono::year_month_day> date =
      DateFromText(select_operation->Column<std::string>(0));
  const core::Result<core::OperationType> type =
      OperationTypeFromText(select_operation->Column<std::string>(1));
  if (!date || !type) {
    return Corrupt(id, !date ? date.error().message : type.error().message);
  }
  StoredOperation stored{.date = *date,
                         .type = *type,
                         .comment = select_operation->Column<std::string>(2),
                         .movements = {}};

  // The currency is not stored with a movement: it is the account's currency.
  const core::Status movements_read =
      db_->Prepare(
             "SELECT m.account_id, m.category_id, m.amount_minor, a.currency "
             "FROM movement m JOIN account a ON a.id = m.account_id "
             "WHERE m.operation_id = ?1 ORDER BY m.id",
             id.value())
          .and_then([&](sqlite::Statement select) {
            return select.ForEachRow([&](const sqlite::Statement& row) {
              return ReadMovement(row).transform([&](core::Movement movement) {
                stored.movements.push_back(movement);
              });
            });
          });
  if (!movements_read) {
    return std::unexpected(movements_read.error());
  }

  core::Result<core::OperationDraft> draft = Rebuild(id, std::move(stored));
  if (!draft) {
    // Errors from the factories are input errors; here they mean the stored
    // data is damaged.
    if (draft.error().code == core::ErrorCode::kStorage) {
      return std::unexpected(std::move(draft).error());
    }
    return Corrupt(id, draft.error().message);
  }
  return core::Operation{.id = id, .draft = std::move(*draft)};
}

core::Result<std::vector<core::Operation>> SqliteOperationRepository::List(
    const core::OperationFilter& filter) {
  // Step 1: the ids, in journal order. Every filter part is optional: a NULL
  // parameter makes its condition true.
  const core::Result<std::vector<core::OperationId>> ids =
      internal::QueryAll<core::OperationId>(
          *db_,
          "SELECT o.id FROM operation o "
          "WHERE (?1 IS NULL OR o.date >= ?1) AND (?2 IS NULL OR o.date <= ?2) "
          "AND (?3 IS NULL OR EXISTS (SELECT 1 FROM movement m "
          "WHERE m.operation_id = o.id AND m.account_id = ?3)) "
          "ORDER BY o.date, o.id",
          [](const sqlite::Statement& row) -> core::Result<core::OperationId> {
            return core::OperationId(row.Column<std::int64_t>(0));
          },
          filter.from.transform(DateToText), filter.to.transform(DateToText),
          filter.account_id.transform(
              [](core::AccountId id) { return id.value(); }));
  if (!ids) {
    return std::unexpected(ids.error());
  }

  // Step 2: load each one through Get(), which validates it again.
  // (One query per operation - fast enough for a personal database; can
  // become a single JOIN if a journal ever gets slow.)
  std::vector<core::Operation> operations;
  operations.reserve(ids->size());
  for (const core::OperationId id : *ids) {
    core::Result<core::Operation> operation = Get(id);
    if (!operation) {
      return std::unexpected(std::move(operation).error());
    }
    operations.push_back(std::move(*operation));
  }
  return operations;
}

core::Result<std::int64_t> SqliteOperationRepository::SumMovements(
    core::AccountId account_id,
    std::optional<std::chrono::year_month_day> up_to) {
  const std::optional<std::string> up_to_text = up_to.transform(DateToText);
  core::Result<sqlite::Statement> select = db_->Prepare(
      "SELECT COALESCE(SUM(m.amount_minor), 0) "
      "FROM movement m JOIN operation o ON o.id = m.operation_id "
      "WHERE m.account_id = ?1 AND (?2 IS NULL OR o.date <= ?2)",
      account_id.value(), up_to_text);
  if (!select) {
    return std::unexpected(std::move(select).error());
  }
  return select->Step().transform([&](bool /*has_row*/) {
    // SUM always returns exactly one row.
    return select->Column<std::int64_t>(0);
  });
}

}  // namespace mymoney::storage
