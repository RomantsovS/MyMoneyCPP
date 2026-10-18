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

// Categories form a tree: "Food" > "Restaurants". A root category has depth 1
// (ADR-0006).
inline constexpr int kMaxCategoryDepth = 2;

// A category that is not stored yet - it has no id (ADR-0012).
struct NewCategory {
  std::string name;
  CategoryKind kind;
  std::optional<CategoryId> parent_id;  // nullopt: a root category
};

// A stored category.
struct Category {
  CategoryId id;
  std::string name;
  CategoryKind kind;
  std::optional<CategoryId> parent_id;

  friend bool operator==(const Category&, const Category&) = default;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_CATEGORY_H_
