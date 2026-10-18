#ifndef MYMONEY_CORE_CURRENCY_H_
#define MYMONEY_CORE_CURRENCY_H_

#include "core/currency_code.h"
#include "core/error.h"

namespace mymoney::core {

// A currency known to the application, with the number of digits after the
// decimal point of its minor unit: EUR 2 (cents), JPY 0, KWD 3.
class Currency {
 public:
  static constexpr int kMaxMinorDigits = 4;

  [[nodiscard]] static Result<Currency> Create(CurrencyCode code,
                                               int minor_digits);

  [[nodiscard]] CurrencyCode code() const { return code_; }
  [[nodiscard]] int minor_digits() const { return minor_digits_; }

  friend bool operator==(const Currency&, const Currency&) = default;

 private:
  Currency(CurrencyCode code, int minor_digits)
      : code_(code), minor_digits_(minor_digits) {}

  CurrencyCode code_;
  int minor_digits_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_CURRENCY_H_
