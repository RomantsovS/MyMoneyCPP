#ifndef MYMONEY_STORAGE_SQLITE_OPERATION_REPOSITORY_H_
#define MYMONEY_STORAGE_SQLITE_OPERATION_REPOSITORY_H_

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include "core/error.h"
#include "core/operation.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage {

class SqliteOperationRepository final : public core::OperationRepository {
 public:
  // The database is not owned and must outlive the repository.
  explicit SqliteOperationRepository(sqlite::Database* db);

  // Inserts the operation and its movements in one transaction.
  core::Result<core::Operation> Add(const core::OperationDraft& draft) override;

  // Loads the operation and rebuilds it through the OperationDraft factories,
  // so a damaged row is reported (kStorage) instead of returned (ADR-0013).
  core::Result<core::Operation> Get(core::OperationId id) override;

  core::Result<std::vector<core::Operation>> List(
      const core::OperationFilter& filter) override;

  core::Result<core::Operation> Latest(core::OperationType type) override;

  core::Result<std::vector<core::CategoryId>> RecentCategories(
      core::OperationType type, int limit) override;

  core::Result<std::int64_t> SumMovements(
      core::AccountId account_id,
      std::optional<std::chrono::year_month_day> up_to) override;

 private:
  sqlite::Database* db_;
};

}  // namespace mymoney::storage

#endif  // MYMONEY_STORAGE_SQLITE_OPERATION_REPOSITORY_H_
