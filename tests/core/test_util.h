#ifndef MYMONEY_TESTS_CORE_TEST_UTIL_H_
#define MYMONEY_TESTS_CORE_TEST_UTIL_H_

#include <chrono>
#include <cstdint>
#include <ostream>
#include <string_view>

#include "core/currency_code.h"
#include "core/error.h"
#include "core/money.h"
#include "core/strong_id.h"

namespace mymoney::core {

// GoogleTest finds these PrintTo overloads by argument-dependent lookup and
// uses them to print values in failure messages (otherwise it prints bytes).

inline std::string_view ToString(ErrorCode code) {
  switch (code) {
    case ErrorCode::kInvalidArgument:
      return "kInvalidArgument";
    case ErrorCode::kNotFound:
      return "kNotFound";
    case ErrorCode::kRuleViolation:
      return "kRuleViolation";
    case ErrorCode::kStorage:
      return "kStorage";
  }
  return "?";
}

inline void PrintTo(ErrorCode code, std::ostream* os) { *os << ToString(code); }

inline void PrintTo(const Error& error, std::ostream* os) {
  *os << ToString(error.code) << ": " << error.message;
}

inline void PrintTo(CurrencyCode code, std::ostream* os) { *os << code.code(); }

inline void PrintTo(const Money& money, std::ostream* os) {
  *os << money.minor_units() << " (minor units) " << money.currency().code();
}

template <typename Tag>
void PrintTo(StrongId<Tag> id, std::ostream* os) {
  *os << "id " << id.value();
}

namespace test {

inline constexpr CurrencyCode kEur("EUR");
inline constexpr CurrencyCode kUsd("USD");

constexpr Money Eur(std::int64_t minor_units) { return {minor_units, kEur}; }
constexpr Money Usd(std::int64_t minor_units) { return {minor_units, kUsd}; }

inline std::chrono::year_month_day Date(int year, unsigned month,
                                        unsigned day) {
  return {std::chrono::year{year}, std::chrono::month{month},
          std::chrono::day{day}};
}

}  // namespace test
}  // namespace mymoney::core

#endif  // MYMONEY_TESTS_CORE_TEST_UTIL_H_
