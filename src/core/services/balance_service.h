#ifndef MYMONEY_CORE_SERVICES_BALANCE_SERVICE_H_
#define MYMONEY_CORE_SERVICES_BALANCE_SERVICE_H_

#include <chrono>
#include <optional>

#include "core/error.h"
#include "core/money.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"

namespace mymoney::core {

// Account balances in the account's own currency. Conversion into another
// currency comes with exchange rates in M7 (ADR-0009).
class BalanceService {
 public:
  // The repositories are not owned and must outlive the service.
  BalanceService(AccountRepository* accounts, OperationRepository* operations);

  // The balance at the end of day `on`, or the current balance if nullopt.
  [[nodiscard]] Result<Money> GetBalance(
      AccountId account_id,
      std::optional<std::chrono::year_month_day> on = std::nullopt);

 private:
  AccountRepository* accounts_;
  OperationRepository* operations_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_SERVICES_BALANCE_SERVICE_H_
