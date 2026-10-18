#ifndef MYMONEY_CORE_CURRENCY_CODE_H_
#define MYMONEY_CORE_CURRENCY_CODE_H_

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <string_view>

#include "core/error.h"

namespace mymoney::core {

namespace internal {
// Intentionally never defined and not constexpr. Calling it while a consteval
// function is evaluated makes that evaluation fail, so an invalid currency
// literal becomes a compile error that mentions this name.
void InvalidCurrencyCodeLiteral();
}  // namespace internal

// An ISO 4217 currency code: three uppercase ASCII letters, e.g. "EUR".
class CurrencyCode {
 public:
  // A literal checked at compile time: CurrencyCode("EUR").
  // CurrencyCode("eu") does not compile. Use Parse() for runtime text.
  consteval explicit CurrencyCode(std::string_view code)
      : letters_(LettersOrFail(code)) {}

  // Checks runtime text, e.g. from a file or user input.
  [[nodiscard]] static Result<CurrencyCode> Parse(std::string_view text);

  [[nodiscard]] static constexpr bool IsValid(std::string_view text) {
    return text.size() == kLength && std::ranges::all_of(text, [](char c) {
             return c >= 'A' && c <= 'Z';
           });
  }

  // The code as text, e.g. "EUR". Valid as long as this object lives.
  [[nodiscard]] constexpr std::string_view code() const {
    return {letters_.data(), letters_.size()};
  }

  friend constexpr bool operator==(const CurrencyCode&,
                                   const CurrencyCode&) = default;
  friend constexpr auto operator<=>(const CurrencyCode&,
                                    const CurrencyCode&) = default;

 private:
  static constexpr std::size_t kLength = 3;
  using Letters = std::array<char, kLength>;

  constexpr explicit CurrencyCode(Letters letters) : letters_(letters) {}

  // Precondition: IsValid(code).
  static constexpr Letters ToLetters(std::string_view code) {
    Letters letters{};
    std::ranges::copy(code, letters.begin());
    return letters;
  }

  static consteval Letters LettersOrFail(std::string_view code) {
    if (!IsValid(code)) {
      internal::InvalidCurrencyCodeLiteral();
    }
    return ToLetters(code);
  }

  Letters letters_;
};

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_CURRENCY_CODE_H_
