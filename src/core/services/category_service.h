#ifndef MYMONEY_CORE_SERVICES_CATEGORY_SERVICE_H_
#define MYMONEY_CORE_SERVICES_CATEGORY_SERVICE_H_

#include "core/category.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"

namespace mymoney::core {

// Creates categories and groups, enforcing the rules of ADR-0015.
class CategoryService {
 public:
  // The repository is not owned and must outlive the service.
  explicit CategoryService(CategoryRepository* categories);

  // The name must not be blank and not used by any other category or group
  // (names are unique across the whole tree, ADR-0014). A group is always at
  // the root. A category with a parent needs an existing group of the same
  // kind as that parent.
  [[nodiscard]] Result<Category> Create(const NewCategory& category);

 private:
  CategoryRepository* categories_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_SERVICES_CATEGORY_SERVICE_H_
