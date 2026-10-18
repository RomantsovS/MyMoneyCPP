#include "core/services/balance_service.h"

#include <chrono>
#include <cstdint>
#include <optional>

#include "core/account.h"
#include "core/check.h"
#include "core/error.h"
#include "core/money.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"

namespace mymoney::core {

BalanceService::BalanceService(AccountRepository* accounts,
                               OperationRepository* operations)
    : accounts_(accounts), operations_(operations) {
  Check(accounts_ != nullptr, "BalanceService needs an AccountRepository");
  Check(operations_ != nullptr, "BalanceService needs an OperationRepository");
}

Result<Money> BalanceService::GetBalance(
    AccountId account_id, std::optional<std::chrono::year_month_day> on) {
  return accounts_->Get(account_id).and_then([&](const Account& account) {
    return operations_->SumMovements(account_id, on)
        .transform(
            [&](std::int64_t sum) { return Money(sum, account.currency); });
  });
}

}  // namespace mymoney::core
