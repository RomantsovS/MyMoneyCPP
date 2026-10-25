#include "core/services/category_service.h"

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"
#include "core/strong_id.h"
#include "core/validation.h"

namespace mymoney::core {

CategoryService::CategoryService(CategoryRepository* categories)
    : categories_(categories) {
  Check(categories_ != nullptr, "CategoryService needs a CategoryRepository");
}

Result<Category> CategoryService::Create(const NewCategory& category) {
  const std::string_view what = category.is_group ? "group" : "category";
  if (Status status = ValidateName(category.name, what).and_then([&] {
        // The existing object may be either, so the message names both.
        return CheckNameIsFree(categories_->GetByName(category.name),
                               "category or group", category.name);
      });
      !status) {
    return std::unexpected(std::move(status).error());
  }
  if (!category.parent_id.has_value()) {
    return categories_->Add(category);  // at the root
  }
  if (category.is_group) {
    return MakeError(
        ErrorCode::kRuleViolation,
        std::format("group '{}' must be at the root", category.name));
  }

  const Result<Category> parent = categories_->Get(*category.parent_id);
  if (!parent) {
    return std::unexpected(parent.error());
  }
  if (!parent->is_group) {
    return MakeError(ErrorCode::kRuleViolation,
                     std::format("'{}' is a category, not a group; only a "
                                 "group can contain categories",
                                 parent->name));
  }
  if (parent->kind != category.kind) {
    return MakeError(ErrorCode::kRuleViolation,
                     std::format("category '{}' must have the same kind as "
                                 "its group '{}'",
                                 category.name, parent->name));
  }
  return categories_->Add(category);
}

Result<Category> CategoryService::Rename(CategoryId id, std::string_view name) {
  Result<Category> category = categories_->Get(id);
  if (!category || category->name == name) {
    return category;  // unknown id, or nothing to change
  }
  const std::string_view what = category->is_group ? "group" : "category";
  return ValidateName(name, what)
      .and_then([&] {
        // The existing object may be either, so the message names both.
        return CheckNameIsFree(categories_->GetByName(name),
                               "category or group", name);
      })
      .and_then([&] { return categories_->Rename(id, name); })
      .transform([&] {
        category->name = std::string(name);
        return std::move(*category);
      });
}

}  // namespace mymoney::core
