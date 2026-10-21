#ifndef MYMONEY_CORE_SERVICES_ACCOUNT_SERVICE_H_
#define MYMONEY_CORE_SERVICES_ACCOUNT_SERVICE_H_

#include <string_view>

#include "core/account.h"
#include "core/error.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/strong_id.h"

namespace mymoney::core {

// Creates and archives accounts, enforcing the account rules.
class AccountService {
 public:
  // The repositories are not owned and must outlive the service.
  AccountService(CurrencyRepository* currencies, AccountRepository* accounts);

  // The name must not be blank and not used by another account; the currency
  // must be known.
  [[nodiscard]] Result<Account> Create(const NewAccount& account);

  // A new name follows the rules of Create(); keeping the old name is fine.
  [[nodiscard]] Result<Account> Rename(AccountId id, std::string_view name);
  // An archived account keeps its history but gets no new operations.
  [[nodiscard]] Status Archive(AccountId id);
  // Makes an archived account usable again.
  [[nodiscard]] Status Unarchive(AccountId id);

 private:
  CurrencyRepository* currencies_;
  AccountRepository* accounts_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_SERVICES_ACCOUNT_SERVICE_H_
