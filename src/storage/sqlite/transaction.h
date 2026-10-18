#ifndef MYMONEY_STORAGE_SQLITE_TRANSACTION_H_
#define MYMONEY_STORAGE_SQLITE_TRANSACTION_H_

#include "core/error.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage::sqlite {

// A database transaction as a scope guard (ADR-0013): everything between
// Begin() and Commit() is stored together - or not at all. If the Transaction
// is destroyed without a successful Commit() (an early error return), its
// changes are rolled back.
class Transaction {
 public:
  // The database is not owned and must outlive the transaction.
  [[nodiscard]] static core::Result<Transaction> Begin(Database& db);

  Transaction(const Transaction&) = delete;
  Transaction& operator=(const Transaction&) = delete;
  Transaction(Transaction&& other) noexcept;
  Transaction& operator=(Transaction&& other) noexcept;
  ~Transaction();

  [[nodiscard]] core::Status Commit();

 private:
  explicit Transaction(Database* db) : db_(db) {}

  void RollbackIfActive() noexcept;

  Database* db_;  // nullptr once committed or moved from
};

}  // namespace mymoney::storage::sqlite

#endif  // MYMONEY_STORAGE_SQLITE_TRANSACTION_H_
