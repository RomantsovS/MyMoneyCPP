#include "core/currency_code.h"

#include <format>
#include <string_view>

#include "core/error.h"

namespace mymoney::core {

Result<CurrencyCode> CurrencyCode::Parse(std::string_view text) {
  if (!IsValid(text)) {
    return MakeError(
        ErrorCode::kInvalidArgument,
        std::format("invalid currency code '{}': expected 3 letters A-Z",
                    text));
  }
  return CurrencyCode(ToLetters(text));
}

}  // namespace mymoney::core
