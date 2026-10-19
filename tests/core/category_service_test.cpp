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

TEST_F(CategoryServiceTest, CreatesGroupsAndCategories) {
  const Category food = AddGroup("Food", CategoryKind::kExpense);
  const Result<Category> restaurants =
      category_service_.Create({.name = "Restaurants",
                                .kind = CategoryKind::kExpense,
                                .parent_id = food.id});
  const Category transport = AddCategory("Transport", CategoryKind::kExpense);

  ASSERT_TRUE(restaurants.has_value()) << restaurants.error().message;
  EXPECT_TRUE(food.is_group);
  EXPECT_EQ(food.parent_id, std::nullopt);
  EXPECT_FALSE(restaurants->is_group);
  EXPECT_EQ(restaurants->parent_id, food.id);
  EXPECT_FALSE(transport.is_group);
  EXPECT_EQ(transport.parent_id, std::nullopt);
}

TEST_F(CategoryServiceTest, RejectsBlankName) {
  for (const bool is_group : {false, true}) {
    const Result<Category> category =
        category_service_.Create({.name = "",
                                  .kind = CategoryKind::kExpense,
                                  .parent_id = std::nullopt,
                                  .is_group = is_group});

    ASSERT_FALSE(category.has_value());
    EXPECT_EQ(category.error().code, ErrorCode::kInvalidArgument);
  }
}

TEST_F(CategoryServiceTest, NamesAreUniqueAcrossGroupsAndCategories) {
  const Category food = AddGroup("Food", CategoryKind::kExpense);
  AddCategory("Other", CategoryKind::kExpense, food.id);

  // Not as a root category, not as a group, not inside the group, not as an
  // income category.
  for (const NewCategory& duplicate : {
           NewCategory{.name = "Other",
                       .kind = CategoryKind::kExpense,
                       .parent_id = std::nullopt},
           NewCategory{.name = "Other",
                       .kind = CategoryKind::kExpense,
                       .parent_id = std::nullopt,
                       .is_group = true},
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

TEST_F(CategoryServiceTest, RejectsUnknownGroup) {
  const Result<Category> category =
      category_service_.Create({.name = "Orphan",
                                .kind = CategoryKind::kExpense,
                                .parent_id = CategoryId(999)});

  ASSERT_FALSE(category.has_value());
  EXPECT_EQ(category.error().code, ErrorCode::kNotFound);
}

TEST_F(CategoryServiceTest, CategoryMustHaveTheKindOfItsGroup) {
  const Category salary = AddGroup("Salary", CategoryKind::kIncome);
  const Result<Category> category =
      category_service_.Create({.name = "Taxes",
                                .kind = CategoryKind::kExpense,
                                .parent_id = salary.id});

  ASSERT_FALSE(category.has_value());
  EXPECT_EQ(category.error().code, ErrorCode::kRuleViolation);
}

TEST_F(CategoryServiceTest, OnlyAGroupCanContainCategories) {
  const Category transport = AddCategory("Transport", CategoryKind::kExpense);
  const Category food = AddGroup("Food", CategoryKind::kExpense);
  const Category restaurants =
      AddCategory("Restaurants", CategoryKind::kExpense, food.id);

  // Neither under a root category nor under a category inside a group.
  for (const CategoryId parent_id : {transport.id, restaurants.id}) {
    const Result<Category> category =
        category_service_.Create({.name = "Taxi",
                                  .kind = CategoryKind::kExpense,
                                  .parent_id = parent_id});

    ASSERT_FALSE(category.has_value());
    EXPECT_EQ(category.error().code, ErrorCode::kRuleViolation);
  }
  EXPECT_EQ(categories_.size(), 3U);  // nothing was stored
}

TEST_F(CategoryServiceTest, GroupIsAlwaysAtTheRoot) {
  const Category food = AddGroup("Food", CategoryKind::kExpense);

  const Result<Category> group =
      category_service_.Create({.name = "Snacks",
                                .kind = CategoryKind::kExpense,
                                .parent_id = food.id,
                                .is_group = true});

  ASSERT_FALSE(group.has_value());
  EXPECT_EQ(group.error().code, ErrorCode::kRuleViolation);
  EXPECT_EQ(categories_.size(), 1U);
}

}  // namespace
}  // namespace mymoney::core::test
