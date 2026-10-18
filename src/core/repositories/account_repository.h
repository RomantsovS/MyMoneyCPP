#ifndef MYMONEY_CORE_REPOSITORIES_ACCOUNT_REPOSITORY_H_
#define MYMONEY_CORE_REPOSITORIES_ACCOUNT_REPOSITORY_H_

#include <vector>

#include "core/account.h"
#include "core/error.h"
#include "core/strong_id.h"

namespace mymoney::core {

// Storage of accounts. Implemented with SQLite in M2.
// Validation is the job of AccountService; a repository only stores.
class AccountRepository {
 public:
  virtual ~AccountRepository() = default;

  // Stores the account and returns it with its new id.
  [[nodiscard]] virtual Result<Account> Add(const NewAccount& account) = 0;
  // kNotFound if there is no account with this id.
  [[nodiscard]] virtual Result<Account> Get(AccountId id) = 0;
  [[nodiscard]] virtual Result<std::vector<Account>> List() = 0;
  // kNotFound if there is no account with this id.
  [[nodiscard]] virtual Status SetArchived(AccountId id, bool archived) = 0;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_REPOSITORIES_ACCOUNT_REPOSITORY_H_
