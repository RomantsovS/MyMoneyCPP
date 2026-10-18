#ifndef MYMONEY_CORE_MONEY_H_
#define MYMONEY_CORE_MONEY_H_

#include <compare>
#include <cstdint>
#include <limits>

#include "core/check.h"
#include "core/currency_code.h"

namespace mymoney::core {

// An amount of money: an integer number of minor units (cents) in one
// currency. Never a floating-point number. May be negative (ADR-0012).
//
// Arithmetic and ordering need both operands in the same currency, and the
// result must fit into int64. Both are preconditions: breaking them is a
// programming error and stops the program (see Check).
class Money {
 public:
  // The largest absolute amount accepted from user input, in minor units
  // (10^15, i.e. 10 trillion units). Keeps every realistic sum far below the
  // int64 limit of about 9.2 * 10^18.
  static constexpr std::int64_t kMaxInputMinorUnits = 1'000'000'000'000'000;

  constexpr Money(std::int64_t minor_units, CurrencyCode currency)
      : minor_units_(minor_units), currency_(currency) {}

  [[nodiscard]] static constexpr Money Zero(CurrencyCode currency) {
    return {0, currency};
  }

  [[nodiscard]] constexpr std::int64_t minor_units() const {
    return minor_units_;
  }
  [[nodiscard]] constexpr CurrencyCode currency() const { return currency_; }

  [[nodiscard]] constexpr bool IsZero() const { return minor_units_ == 0; }
  [[nodiscard]] constexpr bool IsPositive() const { return minor_units_ > 0; }
  [[nodiscard]] constexpr bool IsNegative() const { return minor_units_ < 0; }

  [[nodiscard]] constexpr Money operator-() const {
    Check(minor_units_ != kMin, "Money negation overflows int64");
    return {-minor_units_, currency_};
  }

  constexpr Money& operator+=(const Money& other) {
    CheckSameCurrency(other);
    Check(other.minor_units_ > 0 ? minor_units_ <= kMax - other.minor_units_
                                 : minor_units_ >= kMin - other.minor_units_,
          "Money addition overflows int64");
    minor_units_ += other.minor_units_;
    return *this;
  }

  constexpr Money& operator-=(const Money& other) {
    CheckSameCurrency(other);
    Check(other.minor_units_ > 0 ? minor_units_ >= kMin + other.minor_units_
                                 : minor_units_ <= kMax + other.minor_units_,
          "Money subtraction overflows int64");
    minor_units_ -= other.minor_units_;
    return *this;
  }

  [[nodiscard]] friend constexpr Money operator+(Money left,
                                                 const Money& right) {
    left += right;
    return left;
  }

  [[nodiscard]] friend constexpr Money operator-(Money left,
                                                 const Money& right) {
    left -= right;
    return left;
  }

  // Equal means the same amount in the same currency; amounts in different
  // currencies are simply not equal.
  friend constexpr bool operator==(const Money&, const Money&) = default;

  // Ordering is only defined within one currency (precondition).
  friend constexpr std::strong_ordering operator<=>(const Money& left,
                                                    const Money& right) {
    left.CheckSameCurrency(right);
    return left.minor_units_ <=> right.minor_units_;
  }

 private:
  static constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();
  static constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();

  constexpr void CheckSameCurrency(const Money& other) const {
    Check(currency_ == other.currency_,
          "Money operands must have the same currency");
  }

  std::int64_t minor_units_;
  CurrencyCode currency_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_MONEY_H_
