#ifndef MYMONEY_TESTS_CORE_FAKES_IN_MEMORY_REPOSITORIES_H_
#define MYMONEY_TESTS_CORE_FAKES_IN_MEMORY_REPOSITORIES_H_

// Hand-written fakes of the repository interfaces (ADR-0012): they really
// store data, in memory, so tests can check the resulting state. The SQLite
// repositories of M2 must behave the same way.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/account.h"
#include "core/category.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/operation.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"

namespace mymoney::core::test {

class InMemoryCurrencyRepository final : public CurrencyRepository {
 public:
  Status Add(const Currency& currency) override {
    if (currencies_.contains(currency.code())) {
      return MakeError(
          ErrorCode::kRuleViolation,
          std::format("currency {} already exists", currency.code().code()));
    }
    currencies_.emplace(currency.code(), currency);
    return {};
  }

  Result<Currency> Get(CurrencyCode code) override {
    const auto it = currencies_.find(code);
    if (it == currencies_.end()) {
      return MakeError(ErrorCode::kNotFound,
                       std::format("unknown currency {}", code.code()));
    }
    return it->second;
  }

  Result<std::vector<Currency>> List() override {
    std::vector<Currency> result;
    for (const auto& [code, currency] : currencies_) {
      result.push_back(currency);
    }
    return result;
  }

 private:
  std::map<CurrencyCode, Currency> currencies_;
};

class InMemoryAccountRepository final : public AccountRepository {
 public:
  Result<Account> Add(const NewAccount& account) override {
    // Like the UNIQUE index in SQLite (migration 0002): a storage error.
    if (GetByName(account.name).has_value()) {
      return MakeError(ErrorCode::kStorage,
                       "UNIQUE constraint failed: account.name");
    }
    const AccountId id(next_id_++);
    const Account stored{.id = id,
                         .name = account.name,
                         .currency = account.currency,
                         .archived = false};
    accounts_.emplace(id, stored);
    return stored;
  }

  Result<Account> Get(AccountId id) override {
    const auto it = accounts_.find(id);
    if (it == accounts_.end()) {
      return NotFound(id);
    }
    return it->second;
  }

  Result<Account> GetByName(std::string_view name) override {
    for (const auto& [id, account] : accounts_) {
      if (account.name == name) {
        return account;
      }
    }
    return MakeError(ErrorCode::kNotFound,
                     std::format("no account named '{}'", name));
  }

  Result<std::vector<Account>> List() override {
    std::vector<Account> result;
    for (const auto& [id, account] : accounts_) {
      result.push_back(account);
    }
    return result;
  }

  Status SetArchived(AccountId id, bool archived) override {
    const auto it = accounts_.find(id);
    if (it == accounts_.end()) {
      return NotFound(id);
    }
    it->second.archived = archived;
    return {};
  }

  Status Rename(AccountId id, std::string_view name) override {
    const auto it = accounts_.find(id);
    if (it == accounts_.end()) {
      return NotFound(id);
    }
    // Like the UNIQUE index in SQLite (migration 0002): a storage error.
    if (const Result<Account> other = GetByName(name);
        other.has_value() && other->id != id) {
      return MakeError(ErrorCode::kStorage,
                       "UNIQUE constraint failed: account.name");
    }
    it->second.name = std::string(name);
    return {};
  }

 private:
  static std::unexpected<Error> NotFound(AccountId id) {
    return MakeError(ErrorCode::kNotFound,
                     std::format("no account with id {}", id.value()));
  }

  std::int64_t next_id_ = 1;
  std::map<AccountId, Account> accounts_;
};

class InMemoryCategoryRepository final : public CategoryRepository {
 public:
  Result<Category> Add(const NewCategory& category) override {
    // Like the UNIQUE index in SQLite (migration 0002): a storage error.
    if (GetByName(category.name).has_value()) {
      return MakeError(ErrorCode::kStorage,
                       "UNIQUE constraint failed: category.name");
    }
    // Like the triggers of migration 0003 (ADR-0015).
    if (category.parent_id.has_value()) {
      if (category.is_group) {
        return MakeError(ErrorCode::kStorage, "a group must be at the root");
      }
      const auto parent = categories_.find(*category.parent_id);
      if (parent == categories_.end() || !parent->second.is_group ||
          parent->second.kind != category.kind) {
        return MakeError(
            ErrorCode::kStorage,
            "a category can only be inside a group of the same kind");
      }
    }
    const CategoryId id(next_id_++);
    const Category stored{.id = id,
                          .name = category.name,
                          .kind = category.kind,
                          .parent_id = category.parent_id,
                          .is_group = category.is_group};
    categories_.emplace(id, stored);
    return stored;
  }

  Result<Category> Get(CategoryId id) override {
    const auto it = categories_.find(id);
    if (it == categories_.end()) {
      return MakeError(ErrorCode::kNotFound,
                       std::format("no category with id {}", id.value()));
    }
    return it->second;
  }

  Result<Category> GetByName(std::string_view name) override {
    for (const auto& [id, category] : categories_) {
      if (category.name == name) {
        return category;
      }
    }
    return MakeError(ErrorCode::kNotFound,
                     std::format("no category named '{}'", name));
  }

  Result<std::vector<Category>> List() override {
    std::vector<Category> result;
    for (const auto& [id, category] : categories_) {
      result.push_back(category);
    }
    return result;
  }

  Status Rename(CategoryId id, std::string_view name) override {
    const auto it = categories_.find(id);
    if (it == categories_.end()) {
      return MakeError(ErrorCode::kNotFound,
                       std::format("no category with id {}", id.value()));
    }
    // Like the UNIQUE index in SQLite (migration 0002): a storage error.
    if (const Result<Category> other = GetByName(name);
        other.has_value() && other->id != id) {
      return MakeError(ErrorCode::kStorage,
                       "UNIQUE constraint failed: category.name");
    }
    it->second.name = std::string(name);
    return {};
  }

  [[nodiscard]] std::size_t size() const { return categories_.size(); }

 private:
  std::int64_t next_id_ = 1;
  std::map<CategoryId, Category> categories_;
};

class InMemoryOperationRepository final : public OperationRepository {
 public:
  // Like the database, the fake knows the categories: it refuses movements on
  // groups. `categories` is not owned and must outlive the fake.
  explicit InMemoryOperationRepository(InMemoryCategoryRepository* categories)
      : categories_(categories) {}

  Result<Operation> Add(const OperationDraft& draft) override {
    // Like the trigger of migration 0003 (ADR-0015).
    for (const Movement& movement : draft.movements()) {
      if (movement.category_id.has_value()) {
        const Result<Category> category =
            categories_->Get(*movement.category_id);
        if (category.has_value() && category->is_group) {
          return MakeError(ErrorCode::kStorage,
                           "a group cannot have operations");
        }
      }
    }
    Operation stored{.id = OperationId(next_id_++), .draft = draft};
    operations_.push_back(stored);
    return stored;
  }

  Result<Operation> Get(OperationId id) override {
    const auto it = std::ranges::find(operations_, id, &Operation::id);
    if (it == operations_.end()) {
      return MakeError(ErrorCode::kNotFound,
                       std::format("no operation with id {}", id.value()));
    }
    return *it;
  }

  Result<std::vector<Operation>> List(const OperationFilter& filter) override {
    std::vector<Operation> result;
    for (const Operation& operation : operations_) {
      const OperationDraft& draft = operation.draft;
      if ((filter.from && draft.date() < *filter.from) ||
          (filter.to && draft.date() > *filter.to)) {
        continue;
      }
      if (filter.account_id &&
          std::ranges::none_of(draft.movements(), [&](const Movement& m) {
            return m.account_id == *filter.account_id;
          })) {
        continue;
      }
      result.push_back(operation);
    }
    // Stored in id order; a stable sort by date keeps that order per day.
    std::ranges::stable_sort(result, {}, [](const Operation& operation) {
      return operation.draft.date();
    });
    return result;
  }

  Result<Operation> Latest(OperationType type) override {
    // Stored in id order: the last match is the most recently entered.
    const auto it = std::ranges::find(
        operations_ | std::views::reverse, type,
        [](const Operation& operation) { return operation.draft.type(); });
    if (it == std::ranges::end(operations_ | std::views::reverse)) {
      return MakeError(ErrorCode::kNotFound, "no such operation yet");
    }
    return *it;
  }

  Result<std::vector<CategoryId>> RecentCategories(OperationType type,
                                                   int limit) override {
    std::vector<CategoryId> result;
    for (const Operation& operation : operations_ | std::views::reverse) {
      if (std::cmp_greater_equal(result.size(), limit)) {
        break;
      }
      if (operation.draft.type() != type) {
        continue;
      }
      for (const Movement& movement : operation.draft.movements()) {
        if (movement.category_id.has_value() &&
            std::ranges::find(result, *movement.category_id) == result.end()) {
          result.push_back(*movement.category_id);
        }
      }
    }
    return result;
  }

  Result<std::int64_t> SumMovements(
      AccountId account_id,
      std::optional<std::chrono::year_month_day> up_to) override {
    std::int64_t sum = 0;
    for (const Operation& operation : operations_) {
      if (up_to.has_value() && operation.draft.date() > *up_to) {
        continue;
      }
      for (const Movement& movement : operation.draft.movements()) {
        if (movement.account_id == account_id) {
          sum += movement.amount.minor_units();
        }
      }
    }
    return sum;
  }

  // Fake-only: how many operations are stored.
  [[nodiscard]] std::size_t size() const { return operations_.size(); }

 private:
  InMemoryCategoryRepository* categories_;
  std::int64_t next_id_ = 1;
  std::vector<Operation> operations_;
};

}  // namespace mymoney::core::test

#endif  // MYMONEY_TESTS_CORE_FAKES_IN_MEMORY_REPOSITORIES_H_
