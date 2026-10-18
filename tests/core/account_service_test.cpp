#include "core/services/account_service.h"

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/services_test_base.h"
#include "core/strong_id.h"
#include "core/test_util.h"

namespace mymoney::core::test {
namespace {

using AccountServiceTest = ServicesTestBase;

TEST_F(AccountServiceTest, CreatesAccountWithNewId) {
  const Result<Account> account =
      account_service_.Create({.name = "Wallet", .currency = kEur});

  ASSERT_TRUE(account.has_value()) << account.error().message;
  EXPECT_EQ(account->name, "Wallet");
  EXPECT_EQ(account->currency, kEur);
  EXPECT_FALSE(account->archived);
  EXPECT_EQ(accounts_.Get(account->id), account);
}

TEST_F(AccountServiceTest, RejectsBlankName) {
  const Result<Account> account =
      account_service_.Create({.name = "  ", .currency = kEur});

  ASSERT_FALSE(account.has_value());
  EXPECT_EQ(account.error().code, ErrorCode::kInvalidArgument);
}

TEST_F(AccountServiceTest, RejectsDuplicateName) {
  AddAccount("Cash EUR");

  const Result<Account> account =
      account_service_.Create({.name = "Cash EUR", .currency = kUsd});

  ASSERT_FALSE(account.has_value());
  EXPECT_EQ(account.error().code, ErrorCode::kRuleViolation);
}

TEST_F(AccountServiceTest, NamesAreCaseSensitive) {
  AddAccount("Cash EUR");

  EXPECT_TRUE(account_service_.Create({.name = "cash eur", .currency = kEur})
                  .has_value());
}

TEST_F(AccountServiceTest, RejectsUnknownCurrency) {
  const Result<Account> account = account_service_.Create(
      {.name = "Yen wallet", .currency = CurrencyCode("JPY")});

  ASSERT_FALSE(account.has_value());
  EXPECT_EQ(account.error().code, ErrorCode::kNotFound);
}

TEST_F(AccountServiceTest, ArchivesAccount) {
  const Account account = AddAccount("Old card");

  ASSERT_TRUE(account_service_.Archive(account.id).has_value());
  EXPECT_TRUE(accounts_.Get(account.id)->archived);
}

TEST_F(AccountServiceTest, ArchivingUnknownAccountFails) {
  const Status status = account_service_.Archive(AccountId(999));

  ASSERT_FALSE(status.has_value());
  EXPECT_EQ(status.error().code, ErrorCode::kNotFound);
}

}  // namespace
}  // namespace mymoney::core::test
