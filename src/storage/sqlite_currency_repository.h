#ifndef MYMONEY_STORAGE_SQLITE_CURRENCY_REPOSITORY_H_
#define MYMONEY_STORAGE_SQLITE_CURRENCY_REPOSITORY_H_

#include <vector>

#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/repositories/currency_repository.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage {

class SqliteCurrencyRepository final : public core::CurrencyRepository {
 public:
  // The database is not owned and must outlive the repository.
  explicit SqliteCurrencyRepository(sqlite::Database* db);

  core::Status Add(const core::Currency& currency) override;
  core::Result<core::Currency> Get(core::CurrencyCode code) override;
  core::Result<std::vector<core::Currency>> List() override;

 private:
  sqlite::Database* db_;
};

}  // namespace mymoney::storage

#endif  // MYMONEY_STORAGE_SQLITE_CURRENCY_REPOSITORY_H_
