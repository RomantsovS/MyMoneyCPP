#include "core/currency_code.h"

#include <gtest/gtest.h>

#include "core/currency.h"
#include "core/error.h"
#include "core/test_util.h"

namespace mymoney::core {
namespace {

// Literals are checked by the compiler; CurrencyCode("eu") would not compile.
static_assert(CurrencyCode("EUR").code() == "EUR");
static_assert(CurrencyCode("EUR") == CurrencyCode("EUR"));
static_assert(CurrencyCode("EUR") < CurrencyCode("USD"));

TEST(CurrencyCodeTest, ParsesThreeUppercaseLetters) {
  const Result<CurrencyCode> code = CurrencyCode::Parse("JPY");

  ASSERT_TRUE(code.has_value());
  EXPECT_EQ(code->code(), "JPY");
  EXPECT_EQ(*code, CurrencyCode("JPY"));
}

TEST(CurrencyCodeTest, RejectsInvalidText) {
  for (const char* text : {"", "EU", "EURO", "eur", "E1R", "€UR"}) {
    const Result<CurrencyCode> code = CurrencyCode::Parse(text);

    ASSERT_FALSE(code.has_value()) << "accepted: " << text;
    EXPECT_EQ(code.error().code, ErrorCode::kInvalidArgument);
  }
}

TEST(CurrencyTest, AcceptsMinorDigitsZeroToFour) {
  EXPECT_TRUE(Currency::Create(CurrencyCode("JPY"), 0).has_value());
  EXPECT_TRUE(Currency::Create(test::kEur, 2).has_value());
  EXPECT_TRUE(Currency::Create(CurrencyCode("CLF"), 4).has_value());
}

TEST(CurrencyTest, RejectsMinorDigitsOutOfRange) {
  EXPECT_EQ(Currency::Create(test::kEur, -1).error().code,
            ErrorCode::kInvalidArgument);
  EXPECT_EQ(Currency::Create(test::kEur, 5).error().code,
            ErrorCode::kInvalidArgument);
}

}  // namespace
}  // namespace mymoney::core
