#include "core/services/category_service.h"

#include <optional>

#include <gtest/gtest.h>

#include "core/category.h"
#include "core/error.h"
#include "core/services_test_base.h"
#include "core/strong_id.h"
#include "core/test_util.h"

namespace mymoney::core::test {
namespace {

using CategoryServiceTest = ServicesTestBase;

TEST_F(CategoryServiceTest, CreatesRootAndChildCategories) {
  const Category food = AddCategory("Food", CategoryKind::kExpense);
  const Result<Category> restaurants =
      category_service_.Create({.name = "Restaurants",
                                .kind = CategoryKind::kExpense,
                                .parent_id = food.id});

  ASSERT_TRUE(restaurants.has_value()) << restaurants.error().message;
  EXPECT_EQ(food.parent_id, std::nullopt);
  EXPECT_EQ(restaurants->parent_id, food.id);
}

TEST_F(CategoryServiceTest, RejectsBlankName) {
  const Result<Category> category = category_service_.Create(
      {.name = "", .kind = CategoryKind::kExpense, .parent_id = std::nullopt});

  ASSERT_FALSE(category.has_value());
  EXPECT_EQ(category.error().code, ErrorCode::kInvalidArgument);
}

TEST_F(CategoryServiceTest, NamesAreUniqueAcrossTheWholeTree) {
  const Category food = AddCategory("Food", CategoryKind::kExpense);
  AddCategory("Other", CategoryKind::kExpense, food.id);

  // Not as a root, not under another parent, not as an income category.
  for (const NewCategory& duplicate : {
           NewCategory{.name = "Other",
                       .kind = CategoryKind::kExpense,
                       .parent_id = std::nullopt},
           NewCategory{.name = "Food",
                       .kind = CategoryKind::kExpense,
                       .parent_id = food.id},
           NewCategory{.name = "Other",
                       .kind = CategoryKind::kIncome,
                       .parent_id = std::nullopt},
       }) {
    const Result<Category> category = category_service_.Create(duplicate);
    ASSERT_FALSE(category.has_value()) << duplicate.name;
    EXPECT_EQ(category.error().code, ErrorCode::kRuleViolation);
  }
}

TEST_F(CategoryServiceTest, RejectsUnknownParent) {
  const Result<Category> category =
      category_service_.Create({.name = "Orphan",
                                .kind = CategoryKind::kExpense,
                                .parent_id = CategoryId(999)});

  ASSERT_FALSE(category.has_value());
  EXPECT_EQ(category.error().code, ErrorCode::kNotFound);
}

TEST_F(CategoryServiceTest, ChildMustHaveTheKindOfItsParent) {
  const Category salary = AddCategory("Salary", CategoryKind::kIncome);
  const Result<Category> category =
      category_service_.Create({.name = "Taxes",
                                .kind = CategoryKind::kExpense,
                                .parent_id = salary.id});

  ASSERT_FALSE(category.has_value());
  EXPECT_EQ(category.error().code, ErrorCode::kRuleViolation);
}

TEST_F(CategoryServiceTest, TreeIsAtMostTwoLevelsDeep) {
  const Category food = AddCategory("Food", CategoryKind::kExpense);
  const Category restaurants =
      AddCategory("Restaurants", CategoryKind::kExpense, food.id);

  const Result<Category> lunch =
      category_service_.Create({.name = "Lunch",
                                .kind = CategoryKind::kExpense,
                                .parent_id = restaurants.id});

  ASSERT_FALSE(lunch.has_value());
  EXPECT_EQ(lunch.error().code, ErrorCode::kRuleViolation);
  EXPECT_EQ(categories_.size(), 2U);  // nothing was stored
}

}  // namespace
}  // namespace mymoney::core::test
