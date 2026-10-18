#ifndef MYMONEY_CORE_OPERATION_H_
#define MYMONEY_CORE_OPERATION_H_

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/error.h"
#include "core/money.h"
#include "core/strong_id.h"

namespace mymoney::core {

enum class OperationType : std::uint8_t {
  kExpense,
  kIncome,
  kTransfer,
};

// One signed change of one account's balance (ADR-0006): positive - money
// comes in, negative - money goes out. The amount is in the currency of the
// account.
struct Movement {
  AccountId account_id;
  std::optional<CategoryId> category_id;  // nullopt for transfers
  Money amount;

  friend bool operator==(const Movement&, const Movement&) = default;
};

// A well-formed operation that is not stored yet (ADR-0012).
//
// It can only be created by the factory functions below, which enforce the
// shape rules of ADR-0006. Rules that need other data (does the account
// exist, does its currency match) are checked by OperationService.
class OperationDraft {
 public:
  // Money spent from an account: one negative movement with an expense
  // category. `amount` is the positive amount spent.
  [[nodiscard]] static Result<OperationDraft> Expense(
      std::chrono::year_month_day date, AccountId account_id,
      CategoryId category_id, Money amount, std::string comment);

  // Money received on an account: one positive movement with an income
  // category.
  [[nodiscard]] static Result<OperationDraft> Income(
      std::chrono::year_month_day date, AccountId account_id,
      CategoryId category_id, Money amount, std::string comment);

  // Money moved between two accounts: a negative movement on the source and a
  // positive one on the target, without categories. In the same currency both
  // amounts must be equal (a fee is a separate expense); between currencies
  // they are independent - the exchange rate of this particular transfer.
  [[nodiscard]] static Result<OperationDraft> Transfer(
      std::chrono::year_month_day date, AccountId from_account_id,
      Money amount_out, AccountId to_account_id, Money amount_in,
      std::string comment);

  [[nodiscard]] std::chrono::year_month_day date() const { return date_; }
  [[nodiscard]] OperationType type() const { return type_; }
  [[nodiscard]] const std::string& comment() const { return comment_; }
  [[nodiscard]] const std::vector<Movement>& movements() const {
    return movements_;
  }

  friend bool operator==(const OperationDraft&,
                         const OperationDraft&) = default;

 private:
  // Shared by Expense and Income: one movement with a category.
  static Result<OperationDraft> SingleMovement(OperationType type,
                                               std::chrono::year_month_day date,
                                               AccountId account_id,
                                               CategoryId category_id,
                                               Money amount,
                                               std::string comment);

  OperationDraft(std::chrono::year_month_day date, OperationType type,
                 std::string comment, std::vector<Movement> movements)
      : date_(date),
        type_(type),
        comment_(std::move(comment)),
        movements_(std::move(movements)) {}

  std::chrono::year_month_day date_;
  OperationType type_;
  std::string comment_;
  std::vector<Movement> movements_;
};

// A stored operation: a draft plus the id the storage assigned.
//
// cppcoreguidelines-pro-type-member-init reports `id` as uninitialized - a
// false positive: Operation has no default constructor (OperationId and
// OperationDraft have none), so `id` is always set. The check reports it only
// in translation units that never use Operation's implicit constructors.
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct Operation {
  OperationId id;
  OperationDraft draft;

  friend bool operator==(const Operation&, const Operation&) = default;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_OPERATION_H_
