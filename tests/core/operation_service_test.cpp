#include "core/services/operation_service.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/error.h"
#include "core/operation.h"
#include "core/repositories/operation_repository.h"
#include "core/services_test_base.h"
#include "core/strong_id.h"
#include "core/test_util.h"

namespace mymoney::core::test {
namespace {

class OperationServiceTest : public ServicesTestBase {
 protected:
  Account cash_ = AddAccount("Cash");
  Account card_ = AddAccount("Card");
  Account dollars_ = AddAccount("Dollars", kUsd);
  Category food_ = AddCategory("Food", CategoryKind::kExpense);
  Category salary_ = AddCategory("Salary", CategoryKind::kIncome);
};

TEST_F(OperationServiceTest, PostsExpense) {
  const OperationDraft draft =
      OperationDraft::Expense(Date(2026, 10, 18), cash_.id, food_.id, Eur(1250),
                              "lunch")
          .value();

  const Result<Operation> operation = operation_service_.Post(draft);

  ASSERT_TRUE(operation.has_value()) << operation.error().message;
  EXPECT_EQ(operation->draft, draft);
  EXPECT_EQ(operations_.Get(operation->id), operation);
}

TEST_F(OperationServiceTest, PostsTransferBetweenCurrencies) {
  const OperationDraft draft =
      OperationDraft::Transfer(Date(2026, 10, 18), card_.id, Eur(10000),
                               dollars_.id, Usd(10850), "exchange")
          .value();

  EXPECT_TRUE(operation_service_.Post(draft).has_value());
}

TEST_F(OperationServiceTest, RejectsUnknownAccount) {
  const OperationDraft draft =
      OperationDraft::Expense(Date(2026, 10, 18), AccountId(999), food_.id,
                              Eur(100), "")
          .value();

  const Result<Operation> operation = operation_service_.Post(draft);

  ASSERT_FALSE(operation.has_value());
  EXPECT_EQ(operation.error().code, ErrorCode::kNotFound);
}

TEST_F(OperationServiceTest, RejectsArchivedAccount) {
  ASSERT_TRUE(account_service_.Archive(card_.id).has_value());
  // The archived account is the *target* of the transfer: every movement is
  // checked, not only the first one.
  const OperationDraft draft =
      OperationDraft::Transfer(Date(2026, 10, 18), cash_.id, Eur(100), card_.id,
                               Eur(100), "")
          .value();

  const Result<Operation> operation = operation_service_.Post(draft);

  ASSERT_FALSE(operation.has_value());
  EXPECT_EQ(operation.error().code, ErrorCode::kRuleViolation);
  EXPECT_EQ(operations_.size(), 0U);  // nothing was stored
}

TEST_F(OperationServiceTest, RejectsAmountInAnotherCurrencyThanTheAccount) {
  const OperationDraft draft =
      OperationDraft::Expense(Date(2026, 10, 18), cash_.id, food_.id, Usd(100),
                              "")
          .value();

  const Result<Operation> operation = operation_service_.Post(draft);

  ASSERT_FALSE(operation.has_value());
  EXPECT_EQ(operation.error().code, ErrorCode::kRuleViolation);
}

TEST_F(OperationServiceTest, RejectsCategoryOfTheWrongKind) {
  const OperationDraft draft =
      OperationDraft::Expense(Date(2026, 10, 18), cash_.id, salary_.id,
                              Eur(100), "")
          .value();

  const Result<Operation> operation = operation_service_.Post(draft);

  ASSERT_FALSE(operation.has_value());
  EXPECT_EQ(operation.error().code, ErrorCode::kRuleViolation);
}

TEST_F(OperationServiceTest, RejectsGroupAsCategory) {
  const Category bills = AddGroup("Bills", CategoryKind::kExpense);
  const OperationDraft draft =
      OperationDraft::Expense(Date(2026, 10, 18), cash_.id, bills.id, Eur(100),
                              "")
          .value();

  const Result<Operation> operation = operation_service_.Post(draft);

  ASSERT_FALSE(operation.has_value());
  EXPECT_EQ(operation.error().code, ErrorCode::kRuleViolation);
  EXPECT_EQ(operations_.size(), 0U);  // nothing was stored
}

TEST_F(OperationServiceTest, RejectsUnknownCategory) {
  const OperationDraft draft =
      OperationDraft::Income(Date(2026, 10, 18), cash_.id, CategoryId(999),
                             Eur(100), "")
          .value();

  const Result<Operation> operation = operation_service_.Post(draft);

  ASSERT_FALSE(operation.has_value());
  EXPECT_EQ(operation.error().code, ErrorCode::kNotFound);
}

// --- Update and Delete (ADR-0019)

TEST_F(OperationServiceTest, UpdatesAnOperationInPlace) {
  const Operation posted =
      operation_service_
          .Post(OperationDraft::Expense(Date(2026, 10, 18), cash_.id, food_.id,
                                        Eur(1250), "lunch")
                    .value())
          .value();
  const OperationDraft changed =
      OperationDraft::Expense(Date(2026, 10, 17), card_.id, food_.id, Eur(1500),
                              "dinner")
          .value();

  const Result<Operation> updated =
      operation_service_.Update(posted.id, changed);

  ASSERT_TRUE(updated.has_value()) << updated.error().message;
  EXPECT_EQ(updated->id, posted.id);
  EXPECT_EQ(operations_.Get(posted.id)->draft, changed);
  EXPECT_EQ(operations_.size(), 1U);
}

TEST_F(OperationServiceTest, UpdateChecksTheNewDraftLikePost) {
  const Operation posted =
      operation_service_
          .Post(OperationDraft::Expense(Date(2026, 10, 18), cash_.id, food_.id,
                                        Eur(100), "")
                    .value())
          .value();
  // An income category for an expense.
  const Result<Operation> updated = operation_service_.Update(
      posted.id, OperationDraft::Expense(Date(2026, 10, 18), cash_.id,
                                         salary_.id, Eur(100), "")
                     .value());

  ASSERT_FALSE(updated.has_value());
  EXPECT_EQ(updated.error().code, ErrorCode::kRuleViolation);
  EXPECT_EQ(operations_.Get(posted.id)->draft.movements().front().category_id,
            food_.id);
}

TEST_F(OperationServiceTest, OperationsOfArchivedAccountsAreFrozen) {
  const Operation posted =
      operation_service_
          .Post(OperationDraft::Transfer(Date(2026, 10, 18), cash_.id, Eur(100),
                                         card_.id, Eur(100), "")
                    .value())
          .value();
  ASSERT_TRUE(account_service_.Archive(card_.id).has_value());

  // The archived account is only the target: still frozen.
  const Result<Operation> updated = operation_service_.Update(
      posted.id, OperationDraft::Expense(Date(2026, 10, 18), cash_.id, food_.id,
                                         Eur(100), "")
                     .value());
  const Status deleted = operation_service_.Delete(posted.id);

  ASSERT_FALSE(updated.has_value());
  EXPECT_EQ(updated.error().code, ErrorCode::kRuleViolation);
  ASSERT_FALSE(deleted.has_value());
  EXPECT_EQ(deleted.error().code, ErrorCode::kRuleViolation);
  EXPECT_EQ(operations_.Get(posted.id)->draft, posted.draft);

  // Unarchived, it can be corrected again.
  ASSERT_TRUE(account_service_.Unarchive(card_.id).has_value());
  EXPECT_TRUE(operation_service_.Delete(posted.id).has_value());
  EXPECT_EQ(operations_.size(), 0U);
}

TEST_F(OperationServiceTest, UpdateCannotMoveAnOperationToAnArchivedAccount) {
  const Operation posted =
      operation_service_
          .Post(OperationDraft::Expense(Date(2026, 10, 18), cash_.id, food_.id,
                                        Eur(100), "")
                    .value())
          .value();
  ASSERT_TRUE(account_service_.Archive(card_.id).has_value());

  const Result<Operation> updated = operation_service_.Update(
      posted.id, OperationDraft::Expense(Date(2026, 10, 18), card_.id, food_.id,
                                         Eur(100), "")
                     .value());

  ASSERT_FALSE(updated.has_value());
  EXPECT_EQ(updated.error().code, ErrorCode::kRuleViolation);
}

TEST_F(OperationServiceTest, UpdateAndDeleteOfAnUnknownOperationAreNotFound) {
  const OperationDraft draft =
      OperationDraft::Expense(Date(2026, 10, 18), cash_.id, food_.id, Eur(100),
                              "")
          .value();

  EXPECT_EQ(operation_service_.Update(OperationId(999), draft).error().code,
            ErrorCode::kNotFound);
  EXPECT_EQ(operation_service_.Delete(OperationId(999)).error().code,
            ErrorCode::kNotFound);
}

// GoogleMock example: here the *interaction* is the point - a draft that
// breaks a rule must never reach the repository's Add().
class MockOperationRepository : public OperationRepository {
 public:
  MOCK_METHOD(Result<Operation>, Add, (const OperationDraft& draft),
              (override));
  MOCK_METHOD(Result<Operation>, Get, (OperationId id), (override));
  MOCK_METHOD(Result<std::vector<Operation>>, List,
              (const OperationFilter& filter), (override));
  MOCK_METHOD(Result<Operation>, Replace,
              (OperationId id, const OperationDraft& draft), (override));
  MOCK_METHOD(Status, Delete, (OperationId id), (override));
  MOCK_METHOD(Result<Operation>, Latest, (OperationType type), (override));
  MOCK_METHOD(Result<std::vector<CategoryId>>, RecentCategories,
              (OperationType type, int limit), (override));
  MOCK_METHOD(Result<std::int64_t>, SumMovements,
              (AccountId account_id,
               std::optional<std::chrono::year_month_day> up_to),
              (override));
};

TEST_F(OperationServiceTest, NeverStoresAnOperationThatBreaksARule) {
  MockOperationRepository mock_operations;
  OperationService service(&accounts_, &categories_, &mock_operations);
  const OperationDraft draft =
      OperationDraft::Expense(Date(2026, 10, 18), cash_.id, salary_.id,
                              Eur(100), "")
          .value();

  EXPECT_CALL(mock_operations, Add).Times(0);

  EXPECT_FALSE(service.Post(draft).has_value());
}

}  // namespace
}  // namespace mymoney::core::test
