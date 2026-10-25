#ifndef MYMONEY_STORAGE_SQLITE_CATEGORY_REPOSITORY_H_
#define MYMONEY_STORAGE_SQLITE_CATEGORY_REPOSITORY_H_

#include <string_view>
#include <vector>

#include "core/category.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"
#include "core/strong_id.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage {

class SqliteCategoryRepository final : public core::CategoryRepository {
 public:
  // The database is not owned and must outlive the repository.
  explicit SqliteCategoryRepository(sqlite::Database* db);

  core::Result<core::Category> Add(const core::NewCategory& category) override;
  core::Result<core::Category> Get(core::CategoryId id) override;
  core::Result<core::Category> GetByName(std::string_view name) override;
  core::Result<std::vector<core::Category>> List() override;
  core::Status Rename(core::CategoryId id, std::string_view name) override;

 private:
  sqlite::Database* db_;
};

}  // namespace mymoney::storage

#endif  // MYMONEY_STORAGE_SQLITE_CATEGORY_REPOSITORY_H_
