#ifndef MYMONEY_STORAGE_SQLITE_ACCOUNT_REPOSITORY_H_
#define MYMONEY_STORAGE_SQLITE_ACCOUNT_REPOSITORY_H_

#include <string_view>
#include <vector>

#include "core/account.h"
#include "core/error.h"
#include "core/repositories/account_repository.h"
#include "core/strong_id.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage {

class SqliteAccountRepository final : public core::AccountRepository {
 public:
  // The database is not owned and must outlive the repository.
  explicit SqliteAccountRepository(sqlite::Database* db);

  core::Result<core::Account> Add(const core::NewAccount& account) override;
  core::Result<core::Account> Get(core::AccountId id) override;
  core::Result<core::Account> GetByName(std::string_view name) override;
  core::Result<std::vector<core::Account>> List() override;
  core::Status SetArchived(core::AccountId id, bool archived) override;

 private:
  sqlite::Database* db_;
};

}  // namespace mymoney::storage

#endif  // MYMONEY_STORAGE_SQLITE_ACCOUNT_REPOSITORY_H_
