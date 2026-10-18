// Contract tests (ADR-0013): the SAME tests run against the in-memory fakes and
// the SQLite repositories. If the two ever behave differently, a test fails -
// so the fast M1 service tests on fakes stay meaningful for the real app.

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/fakes/in_memory_repositories.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/strong_id.h"
#include "core/test_util.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite_account_repository.h"
#include "storage/sqlite_category_repository.h"
#include "storage/sqlite_currency_repository.h"
#include "storage/sqlite_operation_repository.h"
#include "storage/storage.h"

namespace mymoney::core::test {
namespace {

// The two implementations under test, with the same member names.
struct InMemoryRepositories {
  InMemoryCurrencyRepository currencies;
  InMemoryAccountRepository accounts;
  InMemoryCategoryRepository categories;
  InMemoryOperationRepository operations;
};

struct SqliteRepositories {
  // Declared first: the repositories below point to it.
  storage::sqlite::Database db = storage::OpenStorage(":memory:").value();
  storage::SqliteCurrencyRepository currencies{&db};
  storage::SqliteAccountRepository accounts{&db};
  storage::SqliteCategoryRepository categories{&db};
  storage::SqliteOperationRepository operations{&db};
};

template <typename Repositories>
class RepositoryContractTest : public ::testing::Test {
 protected:
  RepositoryContractTest() {
    repos_.currencies.Add(Currency::Create(kEur, 2).value()).value();
    repos_.currencies.Add(Currency::Create(kUsd, 2).value()).value();
  }

  Account AddAccount(std::string_view name, CurrencyCode currency = kEur) {
    return repos_.accounts
        .Add({.name = std::string(name), .currency = currency})
        .value();
  }

  Category AddCategory(std::string_view name, CategoryKind kind) {
    return repos_.categories
        .Add({.name = std::string(name),
              .kind = kind,
              .parent_id = std::nullopt})
        .value();
  }

  Repositories repos_;
};

// Readable test names: RepositoryContractTest/InMemory.*, .../Sqlite.*
class ImplementationNames {
 public:
  template <typename T>
  static std::string GetName(int /*index*/) {
    return std::is_same_v<T, InMemoryRepositories> ? "InMemory" : "Sqlite";
  }
};

using Implementations =
    ::testing::Types<InMemoryRepositories, SqliteRepositories>;
TYPED_TEST_SUITE(RepositoryContractTest, Implementations, ImplementationNames);

// --- Currencies
// ---------------------------------------------------------------

TYPED_TEST(RepositoryContractTest, GetsStoredCurrency) {
  const Result<Currency> eur = this->repos_.currencies.Get(kEur);

  ASSERT_TRUE(eur.has_value()) << eur.error().message;
  EXPECT_EQ(eur->code(), kEur);
  EXPECT_EQ(eur->minor_digits(), 2);
}

TYPED_TEST(RepositoryContractTest, UnknownCurrencyIsNotFound) {
  const Result<Currency> jpy = this->repos_.currencies.Get(CurrencyCode("JPY"));

  ASSERT_FALSE(jpy.has_value());
  EXPECT_EQ(jpy.error().code, ErrorCode::kNotFound);
}

TYPED_TEST(RepositoryContractTest, AddingAnExistingCurrencyIsARuleViolation) {
  const Status status =
      this->repos_.currencies.Add(Currency::Create(kEur, 2).value());

  ASSERT_FALSE(status.has_value());
  EXPECT_EQ(status.error().code, ErrorCode::kRuleViolation);
}

TYPED_TEST(RepositoryContractTest, ListsCurrenciesByCode) {
  const Result<std::vector<Currency>> list = this->repos_.currencies.List();

  ASSERT_TRUE(list.has_value()) << list.error().message;
  ASSERT_EQ(list->size(), 2U);
  EXPECT_EQ(list->front().code(), kEur);
  EXPECT_EQ(list->back().code(), kUsd);
}

// --- Accounts
// -----------------------------------------------------------------

TYPED_TEST(RepositoryContractTest, StoresAccountsWithDistinctIds) {
  const Account cash = this->AddAccount("Cash EUR");
  const Account card = this->AddAccount("Card USD Bank1", kUsd);

  EXPECT_NE(cash.id, card.id);
  EXPECT_EQ(this->repos_.accounts.Get(cash.id), cash);
  EXPECT_EQ(this->repos_.accounts.Get(card.id), card);
  EXPECT_EQ(card.currency, kUsd);
  EXPECT_FALSE(card.archived);
}

TYPED_TEST(RepositoryContractTest, FindsAccountByExactName) {
  const Account card = this->AddAccount("Card USD Bank1", kUsd);

  EXPECT_EQ(this->repos_.accounts.GetByName("Card USD Bank1"), card);
  const Result<Account> other =
      this->repos_.accounts.GetByName("card usd bank1");
  ASSERT_FALSE(other.has_value());
  EXPECT_EQ(other.error().code, ErrorCode::kNotFound);
}

// The UNIQUE index of migration 0002; the fake mirrors it. (Services report
// duplicates earlier and more clearly, as kRuleViolation.)
TYPED_TEST(RepositoryContractTest, StorageRejectsDuplicateNames) {
  this->AddAccount("Cash EUR");
  this->AddCategory("Food", CategoryKind::kExpense);

  const Result<Account> account =
      this->repos_.accounts.Add({.name = "Cash EUR", .currency = kUsd});
  const Result<Category> category =
      this->repos_.categories.Add({.name = "Food",
                                   .kind = CategoryKind::kIncome,
                                   .parent_id = std::nullopt});

  ASSERT_FALSE(account.has_value());
  EXPECT_EQ(account.error().code, ErrorCode::kStorage);
  ASSERT_FALSE(category.has_value());
  EXPECT_EQ(category.error().code, ErrorCode::kStorage);
}

TYPED_TEST(RepositoryContractTest, UnknownAccountIsNotFound) {
  const Result<Account> account = this->repos_.accounts.Get(AccountId(999));

  ASSERT_FALSE(account.has_value());
  EXPECT_EQ(account.error().code, ErrorCode::kNotFound);
}

TYPED_TEST(RepositoryContractTest, ListsAccountsInCreationOrder) {
  const Account first = this->AddAccount("First");
  const Account second = this->AddAccount("Second");

  EXPECT_EQ(this->repos_.accounts.List(),
            (std::vector<Account>{first, second}));
}

TYPED_TEST(RepositoryContractTest, ArchivesAndRestoresAccount) {
  const Account account = this->AddAccount("Old card");

  ASSERT_TRUE(this->repos_.accounts.SetArchived(account.id, true));
  EXPECT_TRUE(this->repos_.accounts.Get(account.id)->archived);
  ASSERT_TRUE(this->repos_.accounts.SetArchived(account.id, false));
  EXPECT_FALSE(this->repos_.accounts.Get(account.id)->archived);
}

TYPED_TEST(RepositoryContractTest, ArchivingUnknownAccountIsNotFound) {
  const Status status = this->repos_.accounts.SetArchived(AccountId(999), true);

  ASSERT_FALSE(status.has_value());
  EXPECT_EQ(status.error().code, ErrorCode::kNotFound);
}

// --- Categories
// ---------------------------------------------------------------

TYPED_TEST(RepositoryContractTest, StoresCategoryTree) {
  const Category food = this->AddCategory("Food", CategoryKind::kExpense);
  const Result<Category> restaurants =
      this->repos_.categories.Add({.name = "Restaurants",
                                   .kind = CategoryKind::kExpense,
                                   .parent_id = food.id});

  ASSERT_TRUE(restaurants.has_value()) << restaurants.error().message;
  EXPECT_EQ(this->repos_.categories.Get(food.id), food);
  EXPECT_EQ(this->repos_.categories.Get(restaurants->id), restaurants);
  EXPECT_EQ(restaurants->parent_id, food.id);
  EXPECT_EQ(this->repos_.categories.List(),
            (std::vector<Category>{food, *restaurants}));
}

TYPED_TEST(RepositoryContractTest, FindsCategoryByName) {
  const Category food = this->AddCategory("Food", CategoryKind::kExpense);

  EXPECT_EQ(this->repos_.categories.GetByName("Food"), food);
  EXPECT_EQ(this->repos_.categories.GetByName("Drinks").error().code,
            ErrorCode::kNotFound);
}

TYPED_TEST(RepositoryContractTest, UnknownCategoryIsNotFound) {
  const Result<Category> category =
      this->repos_.categories.Get(CategoryId(999));

  ASSERT_FALSE(category.has_value());
  EXPECT_EQ(category.error().code, ErrorCode::kNotFound);
}

// --- Operations
// ---------------------------------------------------------------

TYPED_TEST(RepositoryContractTest, StoresAndLoadsEveryOperationType) {
  const Account cash = this->AddAccount("Cash EUR");
  const Account dollars = this->AddAccount("Cash USD", kUsd);
  const Category food = this->AddCategory("Food", CategoryKind::kExpense);
  const Category salary = this->AddCategory("Salary", CategoryKind::kIncome);

  const std::vector<OperationDraft> drafts{
      OperationDraft::Expense(Date(2026, 10, 18), cash.id, food.id, Eur(1250),
                              "lunch")
          .value(),
      OperationDraft::Income(Date(2026, 10, 1), cash.id, salary.id, Eur(300000),
                             "")
          .value(),
      OperationDraft::Transfer(Date(2026, 10, 3), cash.id, Eur(10000),
                               dollars.id, Usd(10850), "exchange")
          .value(),
  };

  for (const OperationDraft& draft : drafts) {
    const Result<Operation> stored = this->repos_.operations.Add(draft);
    ASSERT_TRUE(stored.has_value()) << stored.error().message;
    EXPECT_EQ(stored->draft, draft);
    EXPECT_EQ(this->repos_.operations.Get(stored->id), stored);
  }
}

TYPED_TEST(RepositoryContractTest, UnknownOperationIsNotFound) {
  const Result<Operation> operation =
      this->repos_.operations.Get(OperationId(999));

  ASSERT_FALSE(operation.has_value());
  EXPECT_EQ(operation.error().code, ErrorCode::kNotFound);
}

TYPED_TEST(RepositoryContractTest, ListsOperationsByDateAndFilter) {
  const Account card = this->AddAccount("Card EUR");
  const Account cash = this->AddAccount("Cash EUR");
  const Category food = this->AddCategory("Food", CategoryKind::kExpense);
  auto& operations = this->repos_.operations;
  // Entered out of date order: the list is ordered by date, then by entry.
  const Operation late =
      operations
          .Add(OperationDraft::Expense(Date(2026, 10, 20), cash.id, food.id,
                                       Eur(300), "late")
                   .value())
          .value();
  const Operation transfer =
      operations
          .Add(OperationDraft::Transfer(Date(2026, 10, 5), card.id, Eur(5000),
                                        cash.id, Eur(5000), "ATM")
                   .value())
          .value();
  const Operation early =
      operations
          .Add(OperationDraft::Expense(Date(2026, 10, 5), card.id, food.id,
                                       Eur(100), "same day, entered later")
                   .value())
          .value();

  EXPECT_EQ(operations.List({}),
            (std::vector<Operation>{transfer, early, late}));
  EXPECT_EQ(operations.List({.from = Date(2026, 10, 6),
                             .to = std::nullopt,
                             .account_id = std::nullopt}),
            (std::vector<Operation>{late}));
  EXPECT_EQ(operations.List({.from = std::nullopt,
                             .to = Date(2026, 10, 5),
                             .account_id = std::nullopt}),
            (std::vector<Operation>{transfer, early}));
  // A transfer belongs to both of its accounts.
  EXPECT_EQ(
      operations.List(
          {.from = std::nullopt, .to = std::nullopt, .account_id = cash.id}),
      (std::vector<Operation>{transfer, late}));
  EXPECT_EQ(operations.List({.from = Date(2026, 11, 1),
                             .to = std::nullopt,
                             .account_id = std::nullopt}),
            (std::vector<Operation>{}));
}

TYPED_TEST(RepositoryContractTest, SumOfAnAccountWithoutMovementsIsZero) {
  const Account cash = this->AddAccount("Cash EUR");

  EXPECT_EQ(this->repos_.operations.SumMovements(cash.id, std::nullopt), 0);
}

TYPED_TEST(RepositoryContractTest, SumsSignedMovementsUpToADate) {
  const Account card = this->AddAccount("Card EUR");
  const Account cash = this->AddAccount("Cash EUR");
  const Category food = this->AddCategory("Food", CategoryKind::kExpense);
  const Category salary = this->AddCategory("Salary", CategoryKind::kIncome);
  auto& operations = this->repos_.operations;
  ASSERT_TRUE(operations.Add(OperationDraft::Income(Date(2026, 10, 1), card.id,
                                                    salary.id, Eur(300000), "")
                                 .value()));
  ASSERT_TRUE(
      operations.Add(OperationDraft::Transfer(Date(2026, 10, 2), card.id,
                                              Eur(5000), cash.id, Eur(5000), "")
                         .value()));
  ASSERT_TRUE(
      operations.Add(OperationDraft::Expense(Date(2026, 10, 15), card.id,
                                             food.id, Eur(2000), "")
                         .value()));

  EXPECT_EQ(operations.SumMovements(card.id, std::nullopt), 293000);
  EXPECT_EQ(operations.SumMovements(cash.id, std::nullopt), 5000);
  EXPECT_EQ(operations.SumMovements(card.id, Date(2026, 10, 14)), 295000);
  EXPECT_EQ(operations.SumMovements(card.id, Date(2026, 9, 30)), 0);
}

}  // namespace
}  // namespace mymoney::core::test
