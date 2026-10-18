#ifndef MYMONEY_CORE_REPOSITORIES_OPERATION_REPOSITORY_H_
#define MYMONEY_CORE_REPOSITORIES_OPERATION_REPOSITORY_H_

#include <chrono>
#include <cstdint>
#include <optional>

#include "core/error.h"
#include "core/operation.h"
#include "core/strong_id.h"

namespace mymoney::core {

// Storage of operations and their movements. Implemented with SQLite in M2.
// Validation is the job of OperationService; a repository only stores.
class OperationRepository {
 public:
  virtual ~OperationRepository() = default;

  // Stores the operation together with all its movements - all or nothing -
  // and returns it with its new id.
  [[nodiscard]] virtual Result<Operation> Add(const OperationDraft& draft) = 0;
  // kNotFound if there is no operation with this id.
  [[nodiscard]] virtual Result<Operation> Get(OperationId id) = 0;

  // The sum of all movement amounts of an account, in minor units of the
  // account's currency, for operations dated on or before `up_to` (all
  // operations if nullopt). In M2 this is one SQL SUM, so balances never need
  // to load the movements themselves.
  [[nodiscard]] virtual Result<std::int64_t> SumMovements(
      AccountId account_id,
      std::optional<std::chrono::year_month_day> up_to) = 0;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_REPOSITORIES_OPERATION_REPOSITORY_H_
