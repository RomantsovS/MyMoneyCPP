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
    const CategoryId id(next_id_++);
    const Category stored{.id = id,
                          .name = category.name,
                          .kind = category.kind,
                          .parent_id = category.parent_id};
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

  Result<std::vector<Category>> List() override {
    std::vector<Category> result;
    for (const auto& [id, category] : categories_) {
      result.push_back(category);
    }
    return result;
  }

  [[nodiscard]] std::size_t size() const { return categories_.size(); }

 private:
  std::int64_t next_id_ = 1;
  std::map<CategoryId, Category> categories_;
};

class InMemoryOperationRepository final : public OperationRepository {
 public:
  Result<Operation> Add(const OperationDraft& draft) override {
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
  std::int64_t next_id_ = 1;
  std::vector<Operation> operations_;
};

}  // namespace mymoney::core::test

#endif  // MYMONEY_TESTS_CORE_FAKES_IN_MEMORY_REPOSITORIES_H_
