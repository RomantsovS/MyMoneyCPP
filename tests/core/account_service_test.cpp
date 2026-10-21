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

TEST_F(AccountServiceTest, UnarchivesAccount) {
  const Account account = AddAccount("Old card");
  ASSERT_TRUE(account_service_.Archive(account.id).has_value());

  ASSERT_TRUE(account_service_.Unarchive(account.id).has_value());
  EXPECT_FALSE(accounts_.Get(account.id)->archived);
}

TEST_F(AccountServiceTest, RenamesAccount) {
  const Account account = AddAccount("Card");

  const Result<Account> renamed = account_service_.Rename(account.id, "Visa");

  ASSERT_TRUE(renamed.has_value()) << renamed.error().message;
  EXPECT_EQ(renamed->name, "Visa");
  EXPECT_EQ(renamed->id, account.id);
  EXPECT_EQ(accounts_.Get(account.id), renamed);
}

TEST_F(AccountServiceTest, RenamingToTheSameNameChangesNothing) {
  const Account account = AddAccount("Card");

  EXPECT_EQ(account_service_.Rename(account.id, "Card"), account);
}

TEST_F(AccountServiceTest, RenameFollowsTheNameRules) {
  const Account card = AddAccount("Card");
  AddAccount("Cash");

  const Result<Account> blank = account_service_.Rename(card.id, " ");
  const Result<Account> taken = account_service_.Rename(card.id, "Cash");
  const Result<Account> unknown = account_service_.Rename(AccountId(999), "X");

  ASSERT_FALSE(blank.has_value());
  EXPECT_EQ(blank.error().code, ErrorCode::kInvalidArgument);
  ASSERT_FALSE(taken.has_value());
  EXPECT_EQ(taken.error().code, ErrorCode::kRuleViolation);
  ASSERT_FALSE(unknown.has_value());
  EXPECT_EQ(unknown.error().code, ErrorCode::kNotFound);
  EXPECT_EQ(accounts_.Get(card.id)->name, "Card");
}

}  // namespace
}  // namespace mymoney::core::test
