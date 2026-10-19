#ifndef MYMONEY_CORE_CATEGORY_H_
#define MYMONEY_CORE_CATEGORY_H_

#include <cstdint>
#include <optional>
#include <string>

#include "core/strong_id.h"

namespace mymoney::core {

enum class CategoryKind : std::uint8_t {
  kExpense,
  kIncome,
};

// Categories are "folders and files" (ADR-0015): a group is always at the
// root and only contains categories; a category has operations and no
// children. A category is at the root or inside a group of the same kind, so
// the tree is at most two levels deep: "Food" > "Restaurants".

// A category or group that is not stored yet - it has no id (ADR-0012).
struct NewCategory {
  std::string name;
  CategoryKind kind;
  std::optional<CategoryId> parent_id;  // nullopt: at the root
  bool is_group = false;
};

// A stored category or group.
struct Category {
  CategoryId id;
  std::string name;
  CategoryKind kind;
  std::optional<CategoryId> parent_id;
  bool is_group = false;

  friend bool operator==(const Category&, const Category&) = default;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_CATEGORY_H_
