#ifndef MYMONEY_CORE_ACCOUNT_H_
#define MYMONEY_CORE_ACCOUNT_H_

#include <string>

#include "core/currency_code.h"
#include "core/strong_id.h"

namespace mymoney::core {

// There is deliberately no account type (cash, card, ...): the name says what
// an account is ("Cash USD", "Card USD Bank1"). Grouping comes later with
// user-defined account tags such as "liquid".

// An account that is not stored yet - it has no id (ADR-0012).
struct NewAccount {
  std::string name;
  CurrencyCode currency;
};

// A stored account. Every account holds money in exactly one currency.
struct Account {
  AccountId id;
  std::string name;
  CurrencyCode currency;
  bool archived = false;  // archived accounts get no new operations

  friend bool operator==(const Account&, const Account&) = default;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_ACCOUNT_H_
