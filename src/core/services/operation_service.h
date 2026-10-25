#ifndef MYMONEY_CORE_SERVICES_OPERATION_SERVICE_H_
#define MYMONEY_CORE_SERVICES_OPERATION_SERVICE_H_

#include "core/error.h"
#include "core/operation.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"

namespace mymoney::core {

// Posts operations: checks the rules that need stored data, then stores the
// operation with its movements in one step (ADR-0006, ADR-0012).
class OperationService {
 public:
  // The repositories are not owned and must outlive the service.
  OperationService(AccountRepository* accounts, CategoryRepository* categories,
                   OperationRepository* operations);

  // For every movement: the account exists, is not archived and has the
  // movement's currency; the category exists, is not a group (ADR-0015) and
  // matches the operation type (expense or income). Nothing is stored if any
  // check fails.
  [[nodiscard]] Result<Operation> Post(const OperationDraft& draft);

  // Makes operation `id` the given draft (ADR-0019). The new draft follows
  // the rules of Post(); in addition every account of the operation as it is
  // stored must be active - an archived account freezes its history.
  [[nodiscard]] Result<Operation> Update(OperationId id,
                                         const OperationDraft& draft);

  // Removes the operation; all of its accounts must be active (ADR-0019).
  [[nodiscard]] Status Delete(OperationId id);

 private:
  [[nodiscard]] Status CheckMovement(const Movement& movement,
                                     OperationType type);
  // kRuleViolation if one of the operation's accounts is archived.
  [[nodiscard]] Status CheckAccountsActive(const OperationDraft& stored);

  AccountRepository* accounts_;
  CategoryRepository* categories_;
  OperationRepository* operations_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_SERVICES_OPERATION_SERVICE_H_
