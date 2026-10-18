#include "core/services/balance_service.h"

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/error.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/services_test_base.h"
#include "core/strong_id.h"
#include "core/test_util.h"

namespace mymoney::core::test {
namespace {

class BalanceServiceTest : public ServicesTestBase {
 protected:
  void Post(const Result<OperationDraft>& draft) {
    ASSERT_TRUE(draft.has_value()) << draft.error().message;
    const Result<Operation> posted = operation_service_.Post(*draft);
    ASSERT_TRUE(posted.has_value()) << posted.error().message;
  }

  Account cash_ = AddAccount("Cash");
  Account card_ = AddAccount("Card");
  Account dollars_ = AddAccount("Dollars", kUsd);
  Category food_ = AddCategory("Food", CategoryKind::kExpense);
  Category salary_ = AddCategory("Salary", CategoryKind::kIncome);
};

TEST_F(BalanceServiceTest, NewAccountHasZeroBalance) {
  EXPECT_EQ(balance_service_.GetBalance(cash_.id), Money::Zero(kEur));
}

TEST_F(BalanceServiceTest, BalanceIsTheSumOfAllMovements) {
  Post(OperationDraft::Income(Date(2026, 10, 1), card_.id, salary_.id,
                              Eur(300000), "salary"));
  Post(OperationDraft::Transfer(Date(2026, 10, 2), card_.id, Eur(5000),
                                cash_.id, Eur(5000), "ATM"));
  Post(OperationDraft::Expense(Date(2026, 10, 3), cash_.id, food_.id, Eur(1250),
                               "lunch"));

  EXPECT_EQ(balance_service_.GetBalance(card_.id), Eur(295000));
  EXPECT_EQ(balance_service_.GetBalance(cash_.id), Eur(3750));
}

TEST_F(BalanceServiceTest, TransferBetweenCurrenciesChangesBothBalances) {
  Post(OperationDraft::Transfer(Date(2026, 10, 3), card_.id, Eur(10000),
                                dollars_.id, Usd(10850), "exchange"));

  EXPECT_EQ(balance_service_.GetBalance(card_.id), Eur(-10000));
  EXPECT_EQ(balance_service_.GetBalance(dollars_.id), Usd(10850));
}

TEST_F(BalanceServiceTest, BalanceOnADateIgnoresLaterOperations) {
  Post(OperationDraft::Income(Date(2026, 10, 1), card_.id, salary_.id,
                              Eur(300000), ""));
  Post(OperationDraft::Expense(Date(2026, 10, 15), card_.id, food_.id,
                               Eur(2000), ""));

  EXPECT_EQ(balance_service_.GetBalance(card_.id, Date(2026, 10, 14)),
            Eur(300000));
  EXPECT_EQ(balance_service_.GetBalance(card_.id, Date(2026, 10, 15)),
            Eur(298000));
}

TEST_F(BalanceServiceTest, UnknownAccountHasNoBalance) {
  const Result<Money> balance = balance_service_.GetBalance(AccountId(999));

  ASSERT_FALSE(balance.has_value());
  EXPECT_EQ(balance.error().code, ErrorCode::kNotFound);
}

}  // namespace
}  // namespace mymoney::core::test
