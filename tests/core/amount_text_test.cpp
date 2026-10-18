#include "core/amount_text.h"

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "core/error.h"
#include "core/money.h"
#include "core/test_util.h"

namespace mymoney::core {
namespace {

TEST(ParseAmountTest, ParsesWholeAndDecimalAmounts) {
  EXPECT_EQ(ParseAmount("12.50", 2), 1250);
  EXPECT_EQ(ParseAmount("12.5", 2), 1250);
  EXPECT_EQ(ParseAmount("12", 2), 1200);
  EXPECT_EQ(ParseAmount("0.01", 2), 1);
  EXPECT_EQ(ParseAmount("500", 0), 500);
  EXPECT_EQ(ParseAmount("1.2345", 4), 12345);
}

TEST(ParseAmountTest, AcceptsCommaAsDecimalSeparator) {
  EXPECT_EQ(ParseAmount("12,50", 2), 1250);
}

TEST(ParseAmountTest, RejectsMalformedText) {
  for (const char* text : {"", "-5", "+5", "1 000", "1,000.00", "12.", ".5",
                           "12.5.0", "abc", "12e3", "€5"}) {
    const Result<std::int64_t> amount = ParseAmount(text, 2);
    ASSERT_FALSE(amount.has_value()) << "accepted: " << text;
    EXPECT_EQ(amount.error().code, ErrorCode::kInvalidArgument);
  }
}

TEST(ParseAmountTest, RejectsMoreDecimalsThanTheCurrencyHas) {
  EXPECT_FALSE(ParseAmount("12.505", 2).has_value());
  EXPECT_FALSE(ParseAmount("12.5", 0).has_value());
}

TEST(ParseAmountTest, RejectsAmountsAboveTheInputLimit) {
  // 10^15 minor units = 10^13 with 2 decimals.
  EXPECT_EQ(ParseAmount("10000000000000", 2), Money::kMaxInputMinorUnits);
  EXPECT_FALSE(ParseAmount("10000000000000.01", 2).has_value());
  EXPECT_FALSE(ParseAmount("99999999999999999999999", 2).has_value());
}

TEST(FormatAmountTest, WritesExactlyTheCurrencysDecimals) {
  EXPECT_EQ(FormatAmount(1250, 2), "12.50");
  EXPECT_EQ(FormatAmount(5, 2), "0.05");
  EXPECT_EQ(FormatAmount(-123450, 2), "-1234.50");
  EXPECT_EQ(FormatAmount(0, 2), "0.00");
  EXPECT_EQ(FormatAmount(500, 0), "500");
  EXPECT_EQ(FormatAmount(12345, 4), "1.2345");
}

TEST(FormatAmountTest, HandlesTheExtremes) {
  EXPECT_EQ(FormatAmount(std::numeric_limits<std::int64_t>::min(), 2),
            "-92233720368547758.08");
}

TEST(AmountTextTest, ParseAndFormatRoundTrip) {
  for (const char* text : {"0.01", "12.50", "1234567.89"}) {
    EXPECT_EQ(FormatAmount(ParseAmount(text, 2).value(), 2), text);
  }
}

}  // namespace
}  // namespace mymoney::core
