#ifndef MYMONEY_CORE_SERVICES_CATEGORY_SERVICE_H_
#define MYMONEY_CORE_SERVICES_CATEGORY_SERVICE_H_

#include "core/category.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"

namespace mymoney::core {

// Creates categories, enforcing the category tree rules.
class CategoryService {
 public:
  // The repository is not owned and must outlive the service.
  explicit CategoryService(CategoryRepository* categories);

  // The name must not be blank and not used by any other category (names are
  // unique across the whole tree, ADR-0014). A child needs an existing parent
  // of the same kind, and the tree may not get deeper than kMaxCategoryDepth.
  [[nodiscard]] Result<Category> Create(const NewCategory& category);

 private:
  // Depth of an existing category: 1 for a root category.
  [[nodiscard]] Result<int> DepthOf(const Category& category);

  CategoryRepository* categories_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_SERVICES_CATEGORY_SERVICE_H_
