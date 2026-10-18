#include "core/currency.h"

#include <format>

#include "core/currency_code.h"
#include "core/error.h"

namespace mymoney::core {

Result<Currency> Currency::Create(CurrencyCode code, int minor_digits) {
  if (minor_digits < 0 || minor_digits > kMaxMinorDigits) {
    return MakeError(
        ErrorCode::kInvalidArgument,
        std::format("currency {}: minor digits must be 0..{}, got {}",
                    code.code(), kMaxMinorDigits, minor_digits));
  }
  return Currency(code, minor_digits);
}

}  // namespace mymoney::core
