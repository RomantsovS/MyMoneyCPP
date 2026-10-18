#include "core/services/account_service.h"

#include "core/account.h"
#include "core/check.h"
#include "core/currency.h"
#include "core/error.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/strong_id.h"
#include "core/validation.h"

namespace mymoney::core {

AccountService::AccountService(CurrencyRepository* currencies,
                               AccountRepository* accounts)
    : currencies_(currencies), accounts_(accounts) {
  Check(currencies_ != nullptr, "AccountService needs a CurrencyRepository");
  Check(accounts_ != nullptr, "AccountService needs an AccountRepository");
}

Result<Account> AccountService::Create(const NewAccount& account) {
  return ValidateName(account.name, "account")
      .and_then([&] {
        return CheckNameIsFree(accounts_->GetByName(account.name), "account",
                               account.name);
      })
      .and_then([&] { return currencies_->Get(account.currency); })
      .and_then(
          [&](const Currency& /*known*/) { return accounts_->Add(account); });
}

Status AccountService::Archive(AccountId id) {
  return accounts_->SetArchived(id, true);
}

}  // namespace mymoney::core
