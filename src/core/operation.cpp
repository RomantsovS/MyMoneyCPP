#include "core/operation.h"

#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/error.h"
#include "core/money.h"
#include "core/strong_id.h"

namespace mymoney::core {
namespace {

Status ValidateDate(std::chrono::year_month_day date) {
  if (!date.ok()) {
    return MakeError(ErrorCode::kInvalidArgument, "invalid date");
  }
  return {};
}

// `what` names the amount in the error message, e.g. "amount out".
Status ValidateAmount(const Money& amount, std::string_view what) {
  if (!amount.IsPositive()) {
    return MakeError(ErrorCode::kInvalidArgument,
                     std::format("{} must be positive, got {} minor units",
                                 what, amount.minor_units()));
  }
  if (amount.minor_units() > Money::kMaxInputMinorUnits) {
    return MakeError(
        ErrorCode::kInvalidArgument,
        std::format("{} is too large: {} minor units, max {}", what,
                    amount.minor_units(), Money::kMaxInputMinorUnits));
  }
  return {};
}

Status ValidateTransferAccounts(AccountId from_account_id,
                                AccountId to_account_id) {
  if (from_account_id == to_account_id) {
    return MakeError(ErrorCode::kRuleViolation,
                     "a transfer needs two different accounts");
  }
  return {};
}

Status ValidateTransferAmounts(const Money& amount_out,
                               const Money& amount_in) {
  if (amount_out.currency() == amount_in.currency() &&
      amount_out != amount_in) {
    return MakeError(ErrorCode::kRuleViolation,
                     "a transfer in one currency must move equal amounts; "
                     "record a fee as a separate expense");
  }
  return {};
}

}  // namespace

Result<OperationDraft> OperationDraft::Expense(std::chrono::year_month_day date,
                                               AccountId account_id,
                                               CategoryId category_id,
                                               Money amount,
                                               std::string comment) {
  return SingleMovement(OperationType::kExpense, date, account_id, category_id,
                        amount, std::move(comment));
}

Result<OperationDraft> OperationDraft::Income(std::chrono::year_month_day date,
                                              AccountId account_id,
                                              CategoryId category_id,
                                              Money amount,
                                              std::string comment) {
  return SingleMovement(OperationType::kIncome, date, account_id, category_id,
                        amount, std::move(comment));
}

// Each step runs only if the previous one succeeded; the first error is
// returned unchanged (std::expected's monadic operations, C++23).
Result<OperationDraft> OperationDraft::SingleMovement(
    OperationType type, std::chrono::year_month_day date, AccountId account_id,
    CategoryId category_id, Money amount, std::string comment) {
  return ValidateDate(date)
      .and_then([&] { return ValidateAmount(amount, "amount"); })
      .transform([&] {
        // An expense takes money out of the account, an income brings it in.
        const Money signed_amount =
            type == OperationType::kExpense ? -amount : amount;
        std::vector<Movement> movements{{.account_id = account_id,
                                         .category_id = category_id,
                                         .amount = signed_amount}};
        return OperationDraft(date, type, std::move(comment),
                              std::move(movements));
      });
}

Result<OperationDraft> OperationDraft::Transfer(
    std::chrono::year_month_day date, AccountId from_account_id,
    Money amount_out, AccountId to_account_id, Money amount_in,
    std::string comment) {
  return ValidateDate(date)
      .and_then([&] { return ValidateAmount(amount_out, "amount out"); })
      .and_then([&] { return ValidateAmount(amount_in, "amount in"); })
      .and_then([&] {
        return ValidateTransferAccounts(from_account_id, to_account_id);
      })
      .and_then([&] { return ValidateTransferAmounts(amount_out, amount_in); })
      .transform([&] {
        std::vector<Movement> movements{
            {.account_id = from_account_id,
             .category_id = std::nullopt,
             .amount = -amount_out},
            {.account_id = to_account_id,
             .category_id = std::nullopt,
             .amount = amount_in},
        };
        return OperationDraft(date, OperationType::kTransfer,
                              std::move(comment), std::move(movements));
      });
}

}  // namespace mymoney::core
