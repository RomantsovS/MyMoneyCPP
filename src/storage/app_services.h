#ifndef MYMONEY_STORAGE_APP_SERVICES_H_
#define MYMONEY_STORAGE_APP_SERVICES_H_

#include <utility>

#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/services/account_service.h"
#include "core/services/balance_service.h"
#include "core/services/category_service.h"
#include "core/services/operation_service.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite_account_repository.h"
#include "storage/sqlite_category_repository.h"
#include "storage/sqlite_currency_repository.h"
#include "storage/sqlite_operation_repository.h"

namespace mymoney::storage {

// The shared part of both composition roots, the CLI and the Qt app
// (ADR-0016): owns the database connection and wires the SQLite repositories
// into the core services. Members are declared in dependency order, so they
// are created in that order and destroyed in reverse.
// Not copyable or movable: its members point to each other.
class AppServices {
 public:
  explicit AppServices(sqlite::Database db) : db_(std::move(db)) {}

  AppServices(const AppServices&) = delete;
  AppServices& operator=(const AppServices&) = delete;
  AppServices(AppServices&&) = delete;
  AppServices& operator=(AppServices&&) = delete;
  ~AppServices() = default;

  // Non-owning pointers, never null, valid as long as this object lives.
  core::CurrencyRepository* currencies() { return &currencies_; }
  core::AccountRepository* accounts() { return &accounts_; }
  core::CategoryRepository* categories() { return &categories_; }
  core::OperationRepository* operations() { return &operations_; }

  core::AccountService* account_service() { return &account_service_; }
  core::CategoryService* category_service() { return &category_service_; }
  core::OperationService* operation_service() { return &operation_service_; }
  core::BalanceService* balance_service() { return &balance_service_; }

 private:
  sqlite::Database db_;

  SqliteCurrencyRepository currencies_{&db_};
  SqliteAccountRepository accounts_{&db_};
  SqliteCategoryRepository categories_{&db_};
  SqliteOperationRepository operations_{&db_};

  core::AccountService account_service_{&currencies_, &accounts_};
  core::CategoryService category_service_{&categories_};
  core::OperationService operation_service_{&accounts_, &categories_,
                                            &operations_};
  core::BalanceService balance_service_{&accounts_, &operations_};
};

}  // namespace mymoney::storage

#endif  // MYMONEY_STORAGE_APP_SERVICES_H_
