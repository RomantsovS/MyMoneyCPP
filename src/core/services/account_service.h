#ifndef MYMONEY_CORE_SERVICES_ACCOUNT_SERVICE_H_
#define MYMONEY_CORE_SERVICES_ACCOUNT_SERVICE_H_

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

  // The name must not be blank; the currency must be known.
  [[nodiscard]] Result<Account> Create(const NewAccount& account);

  // An archived account keeps its history but gets no new operations.
  [[nodiscard]] Status Archive(AccountId id);

 private:
  CurrencyRepository* currencies_;
  AccountRepository* accounts_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_SERVICES_ACCOUNT_SERVICE_H_
