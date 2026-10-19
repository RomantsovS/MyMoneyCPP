#ifndef MYMONEY_TESTS_CORE_SERVICES_TEST_BASE_H_
#define MYMONEY_TESTS_CORE_SERVICES_TEST_BASE_H_

#include <optional>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/fakes/in_memory_repositories.h"
#include "core/services/account_service.h"
#include "core/services/balance_service.h"
#include "core/services/category_service.h"
#include "core/services/operation_service.h"
#include "core/strong_id.h"
#include "core/test_util.h"

namespace mymoney::core::test {

// Fixture for service tests: fake repositories, the real services wired to
// them (like a composition root would do), and helpers to create test data.
//
// Helpers use Result::value(): in test code an exception from a failed setup
// step is fine - GoogleTest reports it as a test failure.
class ServicesTestBase : public ::testing::Test {
 protected:
  ServicesTestBase() {
    currencies_.Add(Currency::Create(kEur, 2).value()).value();
    currencies_.Add(Currency::Create(kUsd, 2).value()).value();
  }

  Account AddAccount(std::string name, CurrencyCode currency = kEur) {
    return account_service_
        .Create({.name = std::move(name), .currency = currency})
        .value();
  }

  // A category at the root, or inside the group `group_id`.
  Category AddCategory(std::string name, CategoryKind kind,
                       std::optional<CategoryId> group_id = std::nullopt) {
    return category_service_
        .Create({.name = std::move(name), .kind = kind, .parent_id = group_id})
        .value();
  }

  Category AddGroup(std::string name, CategoryKind kind) {
    return category_service_
        .Create({.name = std::move(name),
                 .kind = kind,
                 .parent_id = std::nullopt,
                 .is_group = true})
        .value();
  }

  InMemoryCurrencyRepository currencies_;
  InMemoryAccountRepository accounts_;
  InMemoryCategoryRepository categories_;
  InMemoryOperationRepository operations_{&categories_};

  AccountService account_service_{&currencies_, &accounts_};
  CategoryService category_service_{&categories_};
  OperationService operation_service_{&accounts_, &categories_, &operations_};
  BalanceService balance_service_{&accounts_, &operations_};
};

}  // namespace mymoney::core::test

#endif  // MYMONEY_TESTS_CORE_SERVICES_TEST_BASE_H_
