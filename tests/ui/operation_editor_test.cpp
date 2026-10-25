#include "ui/controllers/operation_editor.h"

#include <optional>

#include <QSignalSpy>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/test_util.h"
#include "ui/qt_print.h"
#include "ui/ui_test_base.h"

namespace mymoney::ui::test {
namespace {

// u"..."_s is a QString.
using Qt::Literals::StringLiterals::operator""_s;

using core::CategoryKind;
using core::test::Date;
using core::test::Eur;
using core::test::kEur;
using core::test::kUsd;

class OperationEditorTest : public UiTest {
 protected:
  OperationEditorTest() {
    AddCurrency(kEur);
    AddCurrency(kUsd);
  }

  [[nodiscard]] std::int64_t Balance(const core::Account& account) {
    return services_.balance_service()
        ->GetBalance(account.id)
        .value()
        .minor_units();
  }

  // An expense entered outside the editor, e.g. with the CLI.
  void PostExpense(const core::Account& account,
                   const core::Category& category) {
    ASSERT_TRUE(services_.operation_service()->Post(
        core::OperationDraft::Expense(Date(2026, 10, 20), account.id,
                                      category.id, Eur(100), "")
            .value()));
  }

  OperationEditor editor_{
      services_.accounts(),          services_.categories(),
      services_.currencies(),        services_.operations(),
      services_.operation_service(), [] { return Date(2026, 10, 25); }};
  QSignalSpy saved_{&editor_, &OperationEditor::saved};
};

TEST_F(OperationEditorTest, WithoutHistoryStartsWithTheFirstActiveAccount) {
  const core::Account old_card = AddAccount("Old card", kEur);
  ASSERT_TRUE(services_.account_service()->Archive(old_card.id));
  AddAccount("Card", kEur);

  editor_.startNew();

  EXPECT_EQ(editor_.type(), u"expense"_s);
  EXPECT_EQ(editor_.accountName(), u"Card"_s);
  EXPECT_EQ(editor_.currencyCode(), u"EUR"_s);
  EXPECT_EQ(editor_.categoryId(), -1);
  EXPECT_TRUE(editor_.isToday());
  EXPECT_TRUE(editor_.recentCategories().isEmpty());
  EXPECT_FALSE(editor_.valid());
}

TEST_F(OperationEditorTest, AddsAnExpense) {
  const core::Account card = AddAccount("Card", kEur);
  const core::Category food = AddCategory("Food", CategoryKind::kExpense);
  editor_.startNew();

  editor_.setAmountText(u"12,50"_s);  // a Russian keyboard's decimal comma
  editor_.chooseCategory(food.id.value());
  editor_.setComment(u" lunch "_s);
  ASSERT_TRUE(editor_.valid());
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  EXPECT_EQ(saved_.count(), 1);
  EXPECT_EQ(editor_.savedText(), u"Expense 12.50 EUR saved"_s);
  EXPECT_EQ(Balance(card), -1250);
  const core::Operation stored =
      services_.operations()->Latest(core::OperationType::kExpense).value();
  EXPECT_EQ(stored.draft.date(), Date(2026, 10, 25));
  EXPECT_EQ(stored.draft.comment(), "lunch");
}

TEST_F(OperationEditorTest, PrefillsFromTheLatestOperation) {
  AddAccount("Card", kEur);
  const core::Account cash = AddAccount("Cash", kEur);
  const core::Category food = AddCategory("Food", CategoryKind::kExpense);
  const core::Category taxi = AddCategory("Taxi", CategoryKind::kExpense);
  PostExpense(cash, taxi);
  PostExpense(cash, food);

  editor_.startNew();

  EXPECT_EQ(editor_.accountId(), cash.id.value());
  // The recent categories, newest first; still none chosen.
  const QVariantList recent = editor_.recentCategories();
  ASSERT_EQ(recent.size(), 2);
  EXPECT_EQ(recent.at(0).toMap().value(u"name"_s).toString(), u"Food"_s);
  EXPECT_EQ(recent.at(0).toMap().value(u"categoryId"_s).toLongLong(),
            food.id.value());
  EXPECT_EQ(recent.at(1).toMap().value(u"name"_s).toString(), u"Taxi"_s);
  EXPECT_EQ(editor_.categoryId(), -1);
}

TEST_F(OperationEditorTest, AnArchivedLatestAccountIsNotPrefilled) {
  const core::Account card = AddAccount("Card", kEur);
  const core::Account cash = AddAccount("Cash", kEur);
  PostExpense(cash, AddCategory("Food", CategoryKind::kExpense));
  ASSERT_TRUE(services_.account_service()->Archive(cash.id));

  editor_.startNew();

  EXPECT_EQ(editor_.accountId(), card.id.value());
}

TEST_F(OperationEditorTest, OnlyCategoriesOfTheRightKind) {
  AddAccount("Card", kEur);
  const core::Category bills = AddGroup("Bills", CategoryKind::kExpense);
  const core::Category rent =
      AddCategory("Rent", CategoryKind::kExpense, bills.id);
  const core::Category salary = AddCategory("Salary", CategoryKind::kIncome);
  editor_.startNew();

  editor_.chooseCategory(bills.id.value());  // a group (ADR-0015)
  EXPECT_EQ(editor_.categoryId(), -1);
  editor_.chooseCategory(salary.id.value());  // income, not expense
  EXPECT_EQ(editor_.categoryId(), -1);
  editor_.chooseCategory(rent.id.value());
  EXPECT_EQ(editor_.categoryName(), u"Rent"_s);

  // Another type: the category no longer fits and is cleared.
  editor_.setType(u"income"_s);
  EXPECT_EQ(editor_.categoryId(), -1);
  editor_.chooseCategory(salary.id.value());
  EXPECT_EQ(editor_.categoryName(), u"Salary"_s);
}

TEST_F(OperationEditorTest, RejectsAmountsThatAreNotPositiveNumbers) {
  AddAccount("Card", kEur);
  const core::Category food = AddCategory("Food", CategoryKind::kExpense);
  editor_.startNew();
  editor_.chooseCategory(food.id.value());

  for (const QString& text :
       {u""_s, u"abc"_s, u"-5"_s, u"0"_s, u"12.345"_s, u"1 000"_s}) {
    editor_.setAmountText(text);
    EXPECT_FALSE(editor_.valid()) << text.toStdString();
  }
  EXPECT_FALSE(editor_.save());
  EXPECT_FALSE(editor_.errorText().isEmpty());
  EXPECT_EQ(saved_.count(), 0);

  editor_.setAmountText(u" 7 "_s);
  EXPECT_TRUE(editor_.valid());
}

TEST_F(OperationEditorTest, TransferInOneCurrency) {
  const core::Account card = AddAccount("Card", kEur);
  const core::Account cash = AddAccount("Cash", kEur);
  editor_.startNew();

  editor_.setType(u"transfer"_s);
  EXPECT_EQ(editor_.accountId(), card.id.value());
  EXPECT_EQ(editor_.toAccountId(), cash.id.value());  // the other account
  EXPECT_FALSE(editor_.needsAmountIn());
  editor_.setAmountText(u"200"_s);
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  EXPECT_EQ(editor_.savedText(), u"Transfer 200.00 EUR saved"_s);
  EXPECT_EQ(Balance(card), -20000);
  EXPECT_EQ(Balance(cash), 20000);
}

TEST_F(OperationEditorTest, TransferBetweenCurrenciesNeedsTheAmountReceived) {
  const core::Account card = AddAccount("Card", kEur);
  const core::Account dollars = AddAccount("Dollars", kUsd);
  editor_.startNew();
  editor_.setType(u"transfer"_s);
  editor_.setAmountText(u"100"_s);

  ASSERT_TRUE(editor_.needsAmountIn());
  EXPECT_EQ(editor_.toCurrencyCode(), u"USD"_s);
  EXPECT_FALSE(editor_.valid());
  editor_.setAmountInText(u"108.50"_s);
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  EXPECT_EQ(Balance(card), -10000);
  EXPECT_EQ(Balance(dollars), 10850);
}

TEST_F(OperationEditorTest, ATransferNeedsTwoDifferentAccounts) {
  const core::Account card = AddAccount("Card", kEur);
  editor_.startNew();
  editor_.setType(u"transfer"_s);
  editor_.setAmountText(u"5"_s);

  EXPECT_EQ(editor_.toAccountId(), -1);  // there is no second account
  EXPECT_FALSE(editor_.valid());
  editor_.chooseToAccount(card.id.value());
  EXPECT_FALSE(editor_.valid());
}

TEST_F(OperationEditorTest, KeepsTheUsersAccountWhenTheTypeChanges) {
  AddAccount("Card", kEur);
  const core::Account cash = AddAccount("Cash", kEur);
  editor_.startNew();

  editor_.chooseAccount(cash.id.value());
  editor_.setType(u"income"_s);

  EXPECT_EQ(editor_.accountId(), cash.id.value());
}

TEST_F(OperationEditorTest, Dates) {
  AddAccount("Card", kEur);
  editor_.startNew();

  editor_.setYesterday();
  EXPECT_TRUE(editor_.isYesterday());
  EXPECT_FALSE(editor_.isToday());
  EXPECT_EQ(editor_.day(), 24);

  editor_.setDate(2026, 2, 30);  // no such day: ignored
  EXPECT_EQ(editor_.month(), 10);

  editor_.setDate(2026, 3, 1);
  EXPECT_EQ(editor_.year(), 2026);
  EXPECT_EQ(editor_.month(), 3);
  EXPECT_EQ(editor_.day(), 1);
  EXPECT_FALSE(editor_.isToday() || editor_.isYesterday());
  EXPECT_EQ(editor_.dateText(), u"3/1/26"_s);  // the test locale: en_US

  editor_.setToday();
  EXPECT_TRUE(editor_.isToday());
}

}  // namespace
}  // namespace mymoney::ui::test
