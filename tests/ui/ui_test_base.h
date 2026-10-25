#ifndef MYMONEY_TESTS_UI_UI_TEST_BASE_H_
#define MYMONEY_TESTS_UI_UI_TEST_BASE_H_

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/category.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/strong_id.h"
#include "core/test_util.h"
#include "storage/app_services.h"
#include "storage/storage.h"

namespace mymoney::ui::test {

// Fixture for the Qt models and editors (ADR-0016): the real services on an
// in-memory SQLite database, like the app wires them, plus helpers to create
// test data. Helpers use value(): a failed setup step throws and GoogleTest
// reports it as a test failure.
class UiTest : public ::testing::Test {
 protected:
  void AddCurrency(core::CurrencyCode code, int minor_digits = 2) {
    services_.currencies()
        ->Add(core::Currency::Create(code, minor_digits).value())
        .value();
  }

  core::Account AddAccount(std::string name, core::CurrencyCode currency) {
    return services_.account_service()
        ->Create({.name = std::move(name), .currency = currency})
        .value();
  }

  // A category at the root, or inside the group `group_id`.
  core::Category AddCategory(
      std::string name, core::CategoryKind kind,
      std::optional<core::CategoryId> group_id = std::nullopt) {
    return services_.category_service()
        ->Create({.name = std::move(name), .kind = kind, .parent_id = group_id})
        .value();
  }

  core::Category AddGroup(std::string name, core::CategoryKind kind) {
    return services_.category_service()
        ->Create({.name = std::move(name),
                  .kind = kind,
                  .parent_id = std::nullopt,
                  .is_group = true})
        .value();
  }

  // Income of `minor_units` on the account, in its currency.
  void AddIncome(const core::Account& account, std::int64_t minor_units) {
    if (!salary_.has_value()) {
      salary_ = services_.category_service()
                    ->Create({.name = "Salary",
                              .kind = core::CategoryKind::kIncome,
                              .parent_id = std::nullopt})
                    .value();
    }
    const core::Result<core::Operation> posted =
        services_.operation_service()->Post(
            core::OperationDraft::Income(
                core::test::Date(2026, 10, 1), account.id, salary_->id,
                core::Money(minor_units, account.currency), "")
                .value());
    ASSERT_TRUE(posted.has_value()) << posted.error().message;
  }

  storage::AppServices services_{storage::OpenStorage(":memory:").value()};

 private:
  std::optional<core::Category> salary_;
};

}  // namespace mymoney::ui::test

#endif  // MYMONEY_TESTS_UI_UI_TEST_BASE_H_
