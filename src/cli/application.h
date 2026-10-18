#ifndef MYMONEY_CLI_APPLICATION_H_
#define MYMONEY_CLI_APPLICATION_H_

#include <chrono>
#include <utility>

#include "cli/context.h"
#include "core/services/account_service.h"
#include "core/services/balance_service.h"
#include "core/services/category_service.h"
#include "core/services/operation_service.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite_account_repository.h"
#include "storage/sqlite_category_repository.h"
#include "storage/sqlite_currency_repository.h"
#include "storage/sqlite_operation_repository.h"

namespace mymoney::cli {

// The composition root of the CLI: owns the database connection and wires the
// SQLite repositories into the services. Members are declared in dependency
// order, so they are created in that order and destroyed in reverse.
// Not copyable or movable: its members point to each other.
class Application {
 public:
  explicit Application(storage::sqlite::Database db) : db_(std::move(db)) {}

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application&&) = delete;
  ~Application() = default;

  [[nodiscard]] Context MakeContext(std::chrono::year_month_day today) {
    return {.currencies = &currencies_,
            .accounts = &accounts_,
            .categories = &categories_,
            .operations = &operations_,
            .account_service = &account_service_,
            .category_service = &category_service_,
            .operation_service = &operation_service_,
            .balance_service = &balance_service_,
            .today = today};
  }

 private:
  storage::sqlite::Database db_;

  storage::SqliteCurrencyRepository currencies_{&db_};
  storage::SqliteAccountRepository accounts_{&db_};
  storage::SqliteCategoryRepository categories_{&db_};
  storage::SqliteOperationRepository operations_{&db_};

  core::AccountService account_service_{&currencies_, &accounts_};
  core::CategoryService category_service_{&categories_};
  core::OperationService operation_service_{&accounts_, &categories_,
                                            &operations_};
  core::BalanceService balance_service_{&accounts_, &operations_};
};

}  // namespace mymoney::cli

#endif  // MYMONEY_CLI_APPLICATION_H_
