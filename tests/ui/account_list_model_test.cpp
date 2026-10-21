#include "ui/models/account_list_model.h"

#include <QAbstractItemModelTester>
#include <QModelIndex>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/test_util.h"
#include "ui/models/currency_list_model.h"
#include "ui/qt_print.h"
#include "ui/ui_test_base.h"

namespace mymoney::ui::test {
namespace {

// u"..."_s is a QString, "..."_L1 a Latin-1 string.
using Qt::Literals::StringLiterals::operator""_L1;
using Qt::Literals::StringLiterals::operator""_s;

using core::test::kEur;
using core::test::kUsd;

class AccountListModelTest : public UiTest {
 protected:
  AccountListModelTest() {
    AddCurrency(kEur);
    AddCurrency(kUsd);
  }

  [[nodiscard]] QVariant Data(int row, int role) const {
    return model_.data(model_.index(row), role);
  }

  AccountListModel model_{services_.accounts(), services_.currencies(),
                          services_.balance_service()};
  // Checks after every change that the model keeps Qt's model contract
  // (row counts, signals around resets, valid indexes); aborts if not.
  QAbstractItemModelTester tester_{
      &model_, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

TEST_F(AccountListModelTest, EmptyUntilThereAreAccounts) {
  model_.reload();

  EXPECT_EQ(model_.rowCount({}), 0);
  EXPECT_TRUE(model_.totals().isEmpty());
  EXPECT_TRUE(model_.errorText().isEmpty());
}

TEST_F(AccountListModelTest, ShowsAccountsWithBalances) {
  const core::Account card = AddAccount("Card EUR", kEur);
  AddAccount("Наличные USD", kUsd);
  AddIncome(card, 300000);

  model_.reload();

  ASSERT_EQ(model_.rowCount({}), 2);
  EXPECT_EQ(Data(0, AccountListModel::kAccountIdRole).toLongLong(),
            card.id.value());
  EXPECT_EQ(Data(0, AccountListModel::kNameRole).toString(), u"Card EUR"_s);
  EXPECT_EQ(Data(0, AccountListModel::kCurrencyRole).toString(), u"EUR"_s);
  EXPECT_EQ(Data(0, AccountListModel::kBalanceTextRole).toString(),
            u"3,000.00 EUR"_s);
  EXPECT_FALSE(Data(0, AccountListModel::kArchivedRole).toBool());
  EXPECT_EQ(Data(1, AccountListModel::kNameRole).toString(), u"Наличные USD"_s);
  EXPECT_EQ(Data(1, AccountListModel::kBalanceTextRole).toString(),
            u"0.00 USD"_s);
}

TEST_F(AccountListModelTest, QmlSeesTheRolesByName) {
  const auto roles = model_.roleNames();

  EXPECT_EQ(roles.value(AccountListModel::kAccountIdRole), "accountId");
  EXPECT_EQ(roles.value(AccountListModel::kNameRole), "name");
  EXPECT_EQ(roles.value(AccountListModel::kBalanceTextRole), "balanceText");
  EXPECT_EQ(roles.value(AccountListModel::kArchivedRole), "archived");
}

TEST_F(AccountListModelTest, TotalsPerCurrencyByCode) {
  const core::Account dollars = AddAccount("Dollars", kUsd);
  const core::Account card = AddAccount("Card", kEur);
  const core::Account cash = AddAccount("Cash", kEur);
  AddIncome(dollars, 10850);
  AddIncome(card, 100000);
  AddIncome(cash, 2550);

  model_.reload();

  EXPECT_EQ(model_.totals(), (QStringList{"1,025.50 EUR"_L1, "108.50 USD"_L1}));
}

TEST_F(AccountListModelTest, HidesArchivedAccountsUnlessShown) {
  const core::Account old_card = AddAccount("Old card", kEur);
  AddAccount("Cash", kEur);
  AddIncome(old_card, 500);
  ASSERT_TRUE(services_.account_service()->Archive(old_card.id));
  model_.reload();
  const QSignalSpy show_changed(&model_,
                                &AccountListModel::showArchivedChanged);

  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Data(0, AccountListModel::kNameRole).toString(), u"Cash"_s);
  // Hidden accounts are not in the totals either.
  EXPECT_EQ(model_.totals(), QStringList{"0.00 EUR"_L1});

  model_.setShowArchived(true);

  EXPECT_EQ(show_changed.count(), 1);
  ASSERT_EQ(model_.rowCount({}), 2);
  EXPECT_TRUE(Data(0, AccountListModel::kArchivedRole).toBool());
  EXPECT_EQ(model_.totals(), QStringList{"5.00 EUR"_L1});
}

TEST_F(AccountListModelTest, CurrencyListByCode) {
  CurrencyListModel currencies(services_.currencies());
  const QAbstractItemModelTester tester(
      &currencies, QAbstractItemModelTester::FailureReportingMode::Fatal);

  currencies.reload();

  ASSERT_EQ(currencies.rowCount({}), 2);
  EXPECT_EQ(currencies.data(currencies.index(0), CurrencyListModel::kCodeRole)
                .toString(),
            u"EUR"_s);
  EXPECT_EQ(
      currencies.data(currencies.index(1), CurrencyListModel::kMinorDigitsRole)
          .toInt(),
      2);
}

}  // namespace
}  // namespace mymoney::ui::test
