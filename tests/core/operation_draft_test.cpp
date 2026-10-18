#include <chrono>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include "core/error.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/strong_id.h"
#include "core/test_util.h"

namespace mymoney::core {
namespace {

using test::Date;
using test::Eur;
using test::Usd;

constexpr AccountId kCash(1);
constexpr AccountId kCard(2);
constexpr CategoryId kFood(10);
constexpr CategoryId kSalary(20);

TEST(OperationDraftTest, ExpenseIsOneNegativeMovementWithCategory) {
  const Result<OperationDraft> draft = OperationDraft::Expense(
      Date(2026, 10, 18), kCash, kFood, Eur(1250), "lunch");

  ASSERT_TRUE(draft.has_value()) << draft.error().message;
  EXPECT_EQ(draft->type(), OperationType::kExpense);
  EXPECT_EQ(draft->date(), Date(2026, 10, 18));
  EXPECT_EQ(draft->comment(), "lunch");
  EXPECT_EQ(
      draft->movements(),
      (std::vector<Movement>{
          {.account_id = kCash, .category_id = kFood, .amount = Eur(-1250)}}));
}

TEST(OperationDraftTest, IncomeIsOnePositiveMovementWithCategory) {
  const Result<OperationDraft> draft = OperationDraft::Income(
      Date(2026, 10, 1), kCard, kSalary, Eur(300000), "");

  ASSERT_TRUE(draft.has_value()) << draft.error().message;
  EXPECT_EQ(draft->type(), OperationType::kIncome);
  EXPECT_EQ(draft->movements(),
            (std::vector<Movement>{{.account_id = kCard,
                                    .category_id = kSalary,
                                    .amount = Eur(300000)}}));
}

TEST(OperationDraftTest, TransferIsTwoMovementsWithoutCategory) {
  const Result<OperationDraft> draft = OperationDraft::Transfer(
      Date(2026, 10, 2), kCard, Eur(5000), kCash, Eur(5000), "ATM");

  ASSERT_TRUE(draft.has_value()) << draft.error().message;
  EXPECT_EQ(draft->type(), OperationType::kTransfer);
  EXPECT_EQ(draft->movements(), (std::vector<Movement>{
                                    {.account_id = kCard,
                                     .category_id = std::nullopt,
                                     .amount = Eur(-5000)},
                                    {.account_id = kCash,
                                     .category_id = std::nullopt,
                                     .amount = Eur(5000)},
                                }));
}

TEST(OperationDraftTest, TransferBetweenCurrenciesHasIndependentAmounts) {
  // 100.00 EUR bought 108.50 USD - the rate of this particular exchange.
  const Result<OperationDraft> draft = OperationDraft::Transfer(
      Date(2026, 10, 3), kCard, Eur(10000), kCash, Usd(10850), "exchange");

  ASSERT_TRUE(draft.has_value()) << draft.error().message;
  EXPECT_EQ(draft->movements().front().amount, Eur(-10000));
  EXPECT_EQ(draft->movements().back().amount, Usd(10850));
}

TEST(OperationDraftTest, RejectsInvalidDate) {
  const auto draft =
      OperationDraft::Expense(Date(2026, 2, 30), kCash, kFood, Eur(100), "");

  ASSERT_FALSE(draft.has_value());
  EXPECT_EQ(draft.error().code, ErrorCode::kInvalidArgument);
}

TEST(OperationDraftTest, RejectsZeroAndNegativeAmounts) {
  for (const Money amount : {Eur(0), Eur(-100)}) {
    const auto expense =
        OperationDraft::Expense(Date(2026, 10, 18), kCash, kFood, amount, "");
    const auto transfer = OperationDraft::Transfer(Date(2026, 10, 18), kCash,
                                                   amount, kCard, amount, "");

    ASSERT_FALSE(expense.has_value());
    EXPECT_EQ(expense.error().code, ErrorCode::kInvalidArgument);
    ASSERT_FALSE(transfer.has_value());
    EXPECT_EQ(transfer.error().code, ErrorCode::kInvalidArgument);
  }
}

TEST(OperationDraftTest, RejectsAmountsAboveTheInputLimit) {
  const Money too_much(Money::kMaxInputMinorUnits + 1, test::kEur);
  const Money limit(Money::kMaxInputMinorUnits, test::kEur);

  EXPECT_FALSE(
      OperationDraft::Income(Date(2026, 1, 1), kCash, kSalary, too_much, "")
          .has_value());
  EXPECT_TRUE(
      OperationDraft::Income(Date(2026, 1, 1), kCash, kSalary, limit, "")
          .has_value());
}

TEST(OperationDraftTest, RejectsTransferToTheSameAccount) {
  const auto draft = OperationDraft::Transfer(Date(2026, 10, 18), kCash,
                                              Eur(100), kCash, Eur(100), "");

  ASSERT_FALSE(draft.has_value());
  EXPECT_EQ(draft.error().code, ErrorCode::kRuleViolation);
}

TEST(OperationDraftTest, RejectsUnequalAmountsInOneCurrency) {
  const auto draft =
      OperationDraft::Transfer(Date(2026, 10, 18), kCard, Eur(10100), kCash,
                               Eur(10000), "transfer with a fee");

  ASSERT_FALSE(draft.has_value());
  EXPECT_EQ(draft.error().code, ErrorCode::kRuleViolation);
}

}  // namespace
}  // namespace mymoney::core
