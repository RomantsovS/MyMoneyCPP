#ifndef MYMONEY_CORE_REPOSITORIES_OPERATION_REPOSITORY_H_
#define MYMONEY_CORE_REPOSITORIES_OPERATION_REPOSITORY_H_

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/error.h"
#include "core/operation.h"
#include "core/strong_id.h"

namespace mymoney::core {

// The order of List() results: by date, then by id (the order of entry).
enum class OperationOrder : std::uint8_t {
  kOldestFirst,  // the CLI's journal
  kNewestFirst,  // the app's journal (ADR-0019)
};

// Which operations List() returns; every part is optional (ADR-0014,
// ADR-0019) and all given parts must match.
struct OperationFilter {
  // Every part has a default, so a designated initializer may name only the
  // parts it needs: List({.category_id = id}).
  std::optional<std::chrono::year_month_day> from = std::nullopt;  // on/after
  std::optional<std::chrono::year_month_day> to = std::nullopt;    // on/before
  std::optional<AccountId> account_id = std::nullopt;  // moves money here
  // With this category - or, for a group, with any of its categories.
  std::optional<CategoryId> category_id = std::nullopt;
  // Whose comment contains this text. Like SQLite's LIKE: case is ignored
  // for Latin letters only; every character matches itself.
  std::optional<std::string> comment_contains = std::nullopt;
  OperationOrder order = OperationOrder::kOldestFirst;
  std::optional<int> limit = std::nullopt;  // at most this many, in `order`
};

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

  // The operations matching `filter`, in `filter.order`.
  [[nodiscard]] virtual Result<std::vector<Operation>> List(
      const OperationFilter& filter) = 0;

  // Makes operation `id` the given draft - its date, type, comment and all
  // movements - in one transaction: all or nothing (ADR-0019). Returns it
  // with the same id; kNotFound if there is no operation with this id.
  [[nodiscard]] virtual Result<Operation> Replace(
      OperationId id, const OperationDraft& draft) = 0;
  // Removes the operation with its movements; kNotFound if there is none.
  [[nodiscard]] virtual Status Delete(OperationId id) = 0;

  // The most recently entered operation of this type - the highest id, not
  // the latest date - so a form can start with its accounts (ADR-0017).
  // kNotFound if there is none.
  [[nodiscard]] virtual Result<Operation> Latest(OperationType type) = 0;

  // The categories of the most recently entered operations of this type,
  // newest first, each one once, at most `limit` (ADR-0017). Empty for
  // transfers, which have no category. Precondition: limit >= 0.
  [[nodiscard]] virtual Result<std::vector<CategoryId>> RecentCategories(
      OperationType type, int limit) = 0;

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
