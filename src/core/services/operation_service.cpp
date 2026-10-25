#include "core/services/operation_service.h"

#include <expected>
#include <format>
#include <utility>

#include "core/account.h"
#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/operation.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"

namespace mymoney::core {
namespace {

// The category kind an operation of this type needs.
CategoryKind RequiredCategoryKind(OperationType type) {
  return type == OperationType::kIncome ? CategoryKind::kIncome
                                        : CategoryKind::kExpense;
}

}  // namespace

OperationService::OperationService(AccountRepository* accounts,
                                   CategoryRepository* categories,
                                   OperationRepository* operations)
    : accounts_(accounts), categories_(categories), operations_(operations) {
  Check(accounts_ != nullptr, "OperationService needs an AccountRepository");
  Check(categories_ != nullptr, "OperationService needs a CategoryRepository");
  Check(operations_ != nullptr,
        "OperationService needs an OperationRepository");
}

Result<Operation> OperationService::Post(const OperationDraft& draft) {
  for (const Movement& movement : draft.movements()) {
    if (Status status = CheckMovement(movement, draft.type()); !status) {
      return std::unexpected(std::move(status).error());
    }
  }
  return operations_->Add(draft);
}

Result<Operation> OperationService::Update(OperationId id,
                                           const OperationDraft& draft) {
  const Result<Operation> stored = operations_->Get(id);
  if (!stored) {
    return std::unexpected(stored.error());
  }
  if (Status status = CheckAccountsActive(stored->draft); !status) {
    return std::unexpected(std::move(status).error());
  }
  for (const Movement& movement : draft.movements()) {
    if (Status status = CheckMovement(movement, draft.type()); !status) {
      return std::unexpected(std::move(status).error());
    }
  }
  return operations_->Replace(id, draft);
}

Status OperationService::Delete(OperationId id) {
  return operations_->Get(id)
      .and_then([&](const Operation& stored) {
        return CheckAccountsActive(stored.draft);
      })
      .and_then([&] { return operations_->Delete(id); });
}

Status OperationService::CheckAccountsActive(const OperationDraft& stored) {
  for (const Movement& movement : stored.movements()) {
    const Result<Account> account = accounts_->Get(movement.account_id);
    if (!account) {
      return std::unexpected(account.error());
    }
    if (account->archived) {
      return MakeError(ErrorCode::kRuleViolation,
                       std::format("account '{}' is archived: unarchive it to "
                                   "change its operations",
                                   account->name));
    }
  }
  return {};
}

Status OperationService::CheckMovement(const Movement& movement,
                                       OperationType type) {
  const Result<Account> account = accounts_->Get(movement.account_id);
  if (!account) {
    return std::unexpected(account.error());
  }
  if (account->archived) {
    return MakeError(
        ErrorCode::kRuleViolation,
        std::format("account '{}' is archived and gets no new operations",
                    account->name));
  }
  if (account->currency != movement.amount.currency()) {
    return MakeError(ErrorCode::kRuleViolation,
                     std::format("account '{}' is in {}, the amount is in {}",
                                 account->name, account->currency.code(),
                                 movement.amount.currency().code()));
  }

  if (!movement.category_id.has_value()) {
    return {};  // transfers have no category
  }
  const Result<Category> category = categories_->Get(*movement.category_id);
  if (!category) {
    return std::unexpected(category.error());
  }
  if (category->is_group) {
    return MakeError(ErrorCode::kRuleViolation,
                     std::format("'{}' is a group; operations need one of "
                                 "its categories",
                                 category->name));
  }
  if (category->kind != RequiredCategoryKind(type)) {
    return MakeError(ErrorCode::kRuleViolation,
                     std::format("category '{}' has the wrong kind for this "
                                 "operation (expense vs. income)",
                                 category->name));
  }
  return {};
}

}  // namespace mymoney::core
