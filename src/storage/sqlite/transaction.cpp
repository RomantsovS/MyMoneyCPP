#include "storage/sqlite/transaction.h"

#include <expected>
#include <utility>

#include "core/check.h"
#include "core/error.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage::sqlite {

core::Result<Transaction> Transaction::Begin(Database& db) {
  // IMMEDIATE takes the write lock at once instead of at the first write.
  if (core::Status status = db.Execute("BEGIN IMMEDIATE"); !status) {
    return std::unexpected(std::move(status).error());
  }
  return Transaction(&db);
}

Transaction::Transaction(Transaction&& other) noexcept
    : db_(std::exchange(other.db_, nullptr)) {}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
  if (this != &other) {
    RollbackIfActive();
    db_ = std::exchange(other.db_, nullptr);
  }
  return *this;
}

Transaction::~Transaction() { RollbackIfActive(); }

core::Status Transaction::Commit() {
  core::Check(db_ != nullptr, "Commit() on a finished transaction");
  core::Status status = db_->Execute("COMMIT");
  if (status) {
    db_ = nullptr;  // only on success; a failed COMMIT is rolled back later
  }
  return status;
}

void Transaction::RollbackIfActive() noexcept {
  if (db_ != nullptr) {
    // Nothing useful can be done if ROLLBACK itself fails.
    static_cast<void>(db_->Execute("ROLLBACK"));
    db_ = nullptr;
  }
}

}  // namespace mymoney::storage::sqlite
