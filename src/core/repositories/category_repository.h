#ifndef MYMONEY_CORE_REPOSITORIES_CATEGORY_REPOSITORY_H_
#define MYMONEY_CORE_REPOSITORIES_CATEGORY_REPOSITORY_H_

#include <string_view>
#include <vector>

#include "core/category.h"
#include "core/error.h"
#include "core/strong_id.h"

namespace mymoney::core {

// Storage of categories. Implemented with SQLite in M2.
// Validation is the job of CategoryService; a repository only stores.
class CategoryRepository {
 public:
  virtual ~CategoryRepository() = default;

  // Stores the category and returns it with its new id.
  [[nodiscard]] virtual Result<Category> Add(const NewCategory& category) = 0;
  // kNotFound if there is no category with this id.
  [[nodiscard]] virtual Result<Category> Get(CategoryId id) = 0;
  // Category names are unique (ADR-0014). kNotFound if there is none.
  [[nodiscard]] virtual Result<Category> GetByName(std::string_view name) = 0;
  [[nodiscard]] virtual Result<std::vector<Category>> List() = 0;
  // Changes only the name (ADR-0018). kNotFound if there is no category with
  // this id.
  [[nodiscard]] virtual Status Rename(CategoryId id, std::string_view name) = 0;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_REPOSITORIES_CATEGORY_REPOSITORY_H_
