#include <QAbstractItemModelTester>
#include <QDate>
#include <QString>
#include <QVariant>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/test_util.h"
#include "ui/date_convert.h"
#include "ui/models/account_picker_model.h"
#include "ui/models/category_picker_model.h"
#include "ui/qt_print.h"
#include "ui/ui_test_base.h"

namespace mymoney::ui::test {
namespace {

// u"..."_s is a QString.
using Qt::Literals::StringLiterals::operator""_s;

using core::CategoryKind;
using core::test::Date;
using core::test::kEur;

class CategoryPickerModelTest : public UiTest {
 protected:
  [[nodiscard]] QString Name(int row) const {
    return model_.data(model_.index(row), CategoryPickerModel::kNameRole)
        .toString();
  }
  [[nodiscard]] QString Group(int row) const {
    return model_.data(model_.index(row), CategoryPickerModel::kGroupNameRole)
        .toString();
  }

  CategoryPickerModel model_{services_.categories()};
  QAbstractItemModelTester tester_{
      &model_, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

TEST_F(CategoryPickerModelTest, CategoriesOfOneKindByGroupThenName) {
  const core::Category food = AddGroup("Food", CategoryKind::kExpense);
  AddCategory("Restaurants", CategoryKind::kExpense, food.id);
  AddCategory("Groceries", CategoryKind::kExpense, food.id);
  const core::Category car = AddGroup("Car", CategoryKind::kExpense);
  AddCategory("Fuel", CategoryKind::kExpense, car.id);
  AddCategory("Taxi", CategoryKind::kExpense);
  AddCategory("Salary", CategoryKind::kIncome);

  model_.load(u"expense"_s);

  // No groups, no income; root categories first, then by group and name.
  ASSERT_EQ(model_.rowCount({}), 4);
  EXPECT_EQ(Name(0), u"Taxi"_s);
  EXPECT_EQ(Group(0), QString());
  EXPECT_EQ(Name(1), u"Fuel"_s);
  EXPECT_EQ(Group(1), u"Car"_s);
  EXPECT_EQ(Name(2), u"Groceries"_s);
  EXPECT_EQ(Name(3), u"Restaurants"_s);
  EXPECT_EQ(Group(3), u"Food"_s);

  model_.load(u"income"_s);
  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Name(0), u"Salary"_s);
}

TEST_F(CategoryPickerModelTest, FiltersByNameOrGroup) {
  const core::Category food = AddGroup("Food", CategoryKind::kExpense);
  AddCategory("Groceries", CategoryKind::kExpense, food.id);
  AddCategory("Cafe", CategoryKind::kExpense, food.id);
  AddCategory("Такси", CategoryKind::kExpense);
  model_.load(u"expense"_s);

  model_.setFilterText(u"caf"_s);
  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Name(0), u"Cafe"_s);

  model_.setFilterText(u"FOOD"_s);  // the group's name, any case
  EXPECT_EQ(model_.rowCount({}), 2);

  model_.setFilterText(u"такс"_s);  // Cyrillic, case-insensitive
  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Name(0), u"Такси"_s);

  model_.load(u"expense"_s);  // opening the picker again clears the filter
  EXPECT_TRUE(model_.filterText().isEmpty());
  EXPECT_EQ(model_.rowCount({}), 3);
}

class AccountPickerModelTest : public UiTest {
 protected:
  AccountPickerModelTest() { AddCurrency(kEur); }

  AccountPickerModel model_{services_.accounts()};
  QAbstractItemModelTester tester_{
      &model_, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

TEST_F(AccountPickerModelTest, OnlyActiveAccounts) {
  const core::Account old_card = AddAccount("Old card", kEur);
  const core::Account card = AddAccount("Card", kEur);
  ASSERT_TRUE(services_.account_service()->Archive(old_card.id));

  model_.reload();

  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(model_.data(model_.index(0), AccountPickerModel::kAccountIdRole)
                .toLongLong(),
            card.id.value());
  EXPECT_EQ(model_.data(model_.index(0), AccountPickerModel::kCurrencyRole)
                .toString(),
            u"EUR"_s);
}

TEST(DateConvertTest, RoundTripsCalendarDates) {
  EXPECT_EQ(ToQDate(Date(2026, 10, 25)), QDate(2026, 10, 25));
  EXPECT_EQ(FromQDate(QDate(2024, 2, 29)), Date(2024, 2, 29));
  EXPECT_EQ(FromQDate(ToQDate(Date(2012, 1, 1))), Date(2012, 1, 1));
}

}  // namespace
}  // namespace mymoney::ui::test
