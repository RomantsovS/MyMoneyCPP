#ifndef MYMONEY_CORE_REPOSITORIES_CURRENCY_REPOSITORY_H_
#define MYMONEY_CORE_REPOSITORIES_CURRENCY_REPOSITORY_H_

#include <vector>

#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"

namespace mymoney::core {

// Storage of the known currencies. Implemented with SQLite in M2.
class CurrencyRepository {
 public:
  virtual ~CurrencyRepository() = default;

  // kRuleViolation if a currency with this code already exists.
  [[nodiscard]] virtual Status Add(const Currency& currency) = 0;
  // kNotFound if the currency is unknown.
  [[nodiscard]] virtual Result<Currency> Get(CurrencyCode code) = 0;
  [[nodiscard]] virtual Result<std::vector<Currency>> List() = 0;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_REPOSITORIES_CURRENCY_REPOSITORY_H_
