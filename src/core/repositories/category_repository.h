#ifndef MYMONEY_CORE_REPOSITORIES_CATEGORY_REPOSITORY_H_
#define MYMONEY_CORE_REPOSITORIES_CATEGORY_REPOSITORY_H_

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
  [[nodiscard]] virtual Result<std::vector<Category>> List() = 0;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_REPOSITORIES_CATEGORY_REPOSITORY_H_
