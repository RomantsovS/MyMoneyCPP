#include "core/services/category_service.h"

#include <expected>
#include <format>
#include <utility>

#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"
#include "core/validation.h"

namespace mymoney::core {

CategoryService::CategoryService(CategoryRepository* categories)
    : categories_(categories) {
  Check(categories_ != nullptr, "CategoryService needs a CategoryRepository");
}

Result<Category> CategoryService::Create(const NewCategory& category) {
  if (Status status = ValidateName(category.name, "category"); !status) {
    return std::unexpected(std::move(status).error());
  }
  if (!category.parent_id.has_value()) {
    return categories_->Add(category);  // a root category
  }

  const Result<Category> parent = categories_->Get(*category.parent_id);
  if (!parent) {
    return std::unexpected(parent.error());
  }
  if (parent->kind != category.kind) {
    return MakeError(ErrorCode::kRuleViolation,
                     std::format("category '{}' must have the same kind as "
                                 "its parent '{}'",
                                 category.name, parent->name));
  }
  const Result<int> parent_depth = DepthOf(*parent);
  if (!parent_depth) {
    return std::unexpected(parent_depth.error());
  }
  if (*parent_depth + 1 > kMaxCategoryDepth) {
    return MakeError(
        ErrorCode::kRuleViolation,
        std::format("category '{}' would be at depth {}; at most "
                    "{} levels are allowed",
                    category.name, *parent_depth + 1, kMaxCategoryDepth));
  }
  return categories_->Add(category);
}

Result<int> CategoryService::DepthOf(const Category& category) {
  int depth = 1;
  Category current = category;
  while (current.parent_id.has_value()) {
    // The service never creates deeper trees, so a longer chain means the
    // stored data is broken (e.g. a cycle) - not a user error.
    Check(depth <= kMaxCategoryDepth, "category tree is deeper than allowed");
    Result<Category> parent = categories_->Get(*current.parent_id);
    if (!parent) {
      return std::unexpected(std::move(parent).error());
    }
    current = std::move(*parent);
    ++depth;
  }
  return depth;
}

}  // namespace mymoney::core
