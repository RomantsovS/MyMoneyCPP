#ifndef MYMONEY_CLI_CONTEXT_H_
#define MYMONEY_CLI_CONTEXT_H_

#include <chrono>

#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/services/account_service.h"
#include "core/services/balance_service.h"
#include "core/services/category_service.h"
#include "core/services/operation_service.h"
#include "storage/app_services.h"

namespace mymoney::cli {

// Everything a command may use (ADR-0014). Non-owning pointers, never null:
// the objects live in a storage::AppServices (or in a test fixture).
// Writes go through services (they check the rules); reads may use
// repositories directly.
struct Context {
  core::CurrencyRepository* currencies;
  core::AccountRepository* accounts;
  core::CategoryRepository* categories;
  core::OperationRepository* operations;

  core::AccountService* account_service;
  core::CategoryService* category_service;
  core::OperationService* operation_service;
  core::BalanceService* balance_service;

  // "Today" for commands without --date; injected so tests are repeatable.
  std::chrono::year_month_day today;
};

// A context for the services of `app`, which must outlive it.
inline Context MakeContext(storage::AppServices& app,
                           std::chrono::year_month_day today) {
  return {.currencies = app.currencies(),
          .accounts = app.accounts(),
          .categories = app.categories(),
          .operations = app.operations(),
          .account_service = app.account_service(),
          .category_service = app.category_service(),
          .operation_service = app.operation_service(),
          .balance_service = app.balance_service(),
          .today = today};
}

}  // namespace mymoney::cli

#endif  // MYMONEY_CLI_CONTEXT_H_
