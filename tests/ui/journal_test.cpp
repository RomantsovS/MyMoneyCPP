#include <chrono>

#include <QAbstractItemModelTester>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/test_util.h"
#include "ui/models/journal_model.h"
#include "ui/qt_print.h"
#include "ui/ui_test_base.h"

namespace mymoney::ui::test {
namespace {

// u"..."_s is a QString.
using Qt::Literals::StringLiterals::operator""_s;

using core::CategoryKind;
using core::OperationDraft;
using core::test::Date;
using core::test::Eur;
using core::test::kEur;

// The currency must exist before the fixture's accounts: a base class is
// constructed before the members of the class derived from it.
class WithEuro : public UiTest {
 protected:
  WithEuro() { AddCurrency(kEur); }
};

class JournalModelTest : public WithEuro {
 protected:
  void Post(const OperationDraft& draft) {
    ASSERT_TRUE(services_.operation_service()->Post(draft));
  }

  void Expense(std::chrono::year_month_day date, const core::Category& category,
               std::int64_t minor_units, std::string comment = "") {
    Post(OperationDraft::Expense(date, card_.id, category.id, Eur(minor_units),
                                 std::move(comment))
             .value());
  }

  [[nodiscard]] QString Data(int row, int role) const {
    return model_.data(model_.index(row), role).toString();
  }

  core::Account card_ = AddAccount("Card", kEur);
  core::Account cash_ = AddAccount("Cash", kEur);
  core::Category food_ = AddGroup("Food", CategoryKind::kExpense);
  core::Category cafe_ = AddCategory("Cafe", CategoryKind::kExpense, food_.id);
  core::Category taxi_ = AddCategory("Taxi", CategoryKind::kExpense);
  core::Category salary_ = AddCategory("Salary", CategoryKind::kIncome);

  // "Today" is Sunday, 25 October 2026.
  JournalModel model_{services_.accounts(), services_.categories(),
                      services_.currencies(), services_.operations(),
                      [] { return Date(2026, 10, 25); }};
  QAbstractItemModelTester tester_{
      &model_, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

TEST_F(JournalModelTest, ShowsTheCurrentMonthNewestFirstByDay) {
  Expense(Date(2026, 9, 30), taxi_, 500);  // last month
  Expense(Date(2026, 10, 3), cafe_, 1250, "lunch");
  Post(OperationDraft::Income(Date(2026, 10, 24), card_.id, salary_.id,
                              Eur(300000), "")
           .value());
  Post(OperationDraft::Transfer(Date(2026, 10, 25), card_.id, Eur(20000),
                                cash_.id, Eur(20000), "ATM")
           .value());

  model_.reload();

  EXPECT_EQ(model_.monthTitle(), u"October 2026"_s);
  ASSERT_EQ(model_.rowCount({}), 3);
  EXPECT_EQ(Data(0, JournalModel::kDayTextRole), u"Today"_s);
  EXPECT_EQ(Data(0, JournalModel::kTitleRole), u"Card → Cash"_s);
  EXPECT_EQ(Data(0, JournalModel::kAccountRole), QString());
  EXPECT_EQ(Data(0, JournalModel::kAmountTextRole), u"200.00 EUR"_s);
  EXPECT_EQ(Data(0, JournalModel::kKindRole), u"transfer"_s);
  EXPECT_EQ(Data(0, JournalModel::kCommentRole), u"ATM"_s);
  EXPECT_EQ(Data(1, JournalModel::kDayTextRole), u"Yesterday"_s);
  EXPECT_EQ(Data(1, JournalModel::kTitleRole), u"Salary"_s);
  EXPECT_EQ(Data(1, JournalModel::kAmountTextRole), u"+3,000.00 EUR"_s);
  EXPECT_EQ(Data(2, JournalModel::kDayTextRole), u"Sat, 3 Oct"_s);
  EXPECT_EQ(Data(2, JournalModel::kAccountRole), u"Card"_s);
  EXPECT_EQ(Data(2, JournalModel::kAmountTextRole), u"-12.50 EUR"_s);

  model_.previousMonth();
  EXPECT_EQ(model_.monthTitle(), u"September 2026"_s);
  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Data(0, JournalModel::kTitleRole), u"Taxi"_s);
}

TEST_F(JournalModelTest, FiltersByAccountAndByCategoryOrGroup) {
  Expense(Date(2026, 10, 1), cafe_, 100);
  Expense(Date(2026, 10, 2), taxi_, 200);
  Post(OperationDraft::Transfer(Date(2026, 10, 3), card_.id, Eur(300), cash_.id,
                                Eur(300), "")
           .value());
  model_.reload();
  ASSERT_EQ(model_.rowCount({}), 3);

  model_.setCategoryId(food_.id.value());  // the group: its Cafe
  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Data(0, JournalModel::kTitleRole), u"Cafe"_s);
  EXPECT_EQ(model_.categoryName(), u"Food"_s);

  model_.setCategoryId(-1);
  model_.setAccountId(cash_.id.value());  // only the transfer touches Cash
  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Data(0, JournalModel::kKindRole), u"transfer"_s);
  EXPECT_EQ(model_.accountName(), u"Cash"_s);
}

TEST_F(JournalModelTest, CommentSearchCoversAllMonths) {
  Expense(Date(2024, 3, 1), cafe_, 100, "Coffee with Anna");
  Expense(Date(2026, 10, 2), cafe_, 200, "coffee");
  Expense(Date(2026, 10, 3), cafe_, 300, "tea");
  model_.reload();

  model_.setCommentText(u" COFFEE "_s);

  EXPECT_TRUE(model_.searching());
  EXPECT_FALSE(model_.truncated());
  ASSERT_EQ(model_.rowCount({}), 2);
  EXPECT_EQ(Data(0, JournalModel::kCommentRole), u"coffee"_s);
  // Another year: the day text has the year.
  EXPECT_EQ(Data(1, JournalModel::kDayTextRole), u"Fri, 1 Mar 2024"_s);

  model_.setCommentText(QString());
  EXPECT_FALSE(model_.searching());
  EXPECT_EQ(model_.rowCount({}), 2);  // October again
}

TEST_F(JournalModelTest, ASearchShowsAtMostTheLimit) {
  for (int i = 0; i <= JournalModel::kSearchLimit; ++i) {
    Expense(Date(2026, 10, 1), cafe_, 100, "x");
  }

  model_.setCommentText(u"x"_s);

  EXPECT_EQ(model_.rowCount({}), JournalModel::kSearchLimit);
  EXPECT_TRUE(model_.truncated());
}

TEST_F(JournalModelTest, AMonthIsNeverCut) {
  // The limit is for searches only: a busy month shows every operation.
  for (int i = 0; i <= JournalModel::kSearchLimit; ++i) {
    Expense(Date(2026, 10, 1), cafe_, 100);
  }

  model_.reload();

  EXPECT_EQ(model_.rowCount({}), JournalModel::kSearchLimit + 1);
  EXPECT_FALSE(model_.truncated());
}

TEST_F(JournalModelTest, OffersEveryAccountAndCategoryAsAFilter) {
  ASSERT_TRUE(services_.account_service()->Archive(cash_.id));
  model_.reload();

  const QVariantList accounts = model_.accountChoices();
  ASSERT_EQ(accounts.size(), 3);  // "any", Card, and the archived Cash
  EXPECT_EQ(accounts.at(0).toMap().value(u"id"_s).toLongLong(), -1);
  EXPECT_EQ(accounts.at(2).toMap().value(u"name"_s).toString(), u"Cash"_s);

  QStringList categories;
  for (const QVariant& choice : model_.categoryChoices()) {
    categories.append(choice.toMap().value(u"name"_s).toString());
  }
  EXPECT_EQ(categories, (QStringList{u"Any category"_s, u"Taxi"_s, u"Food"_s,
                                     u"    Cafe"_s, u"Salary"_s}));
}

}  // namespace
}  // namespace mymoney::ui::test
