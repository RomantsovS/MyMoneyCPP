#include "core/date_text.h"

#include <chrono>

#include <gtest/gtest.h>

#include "core/error.h"
#include "core/test_util.h"

namespace mymoney::core {
namespace {

using test::Date;

TEST(DateTextTest, FormatsAndParsesIsoDates) {
  EXPECT_EQ(FormatDate(Date(2026, 10, 8)), "2026-10-08");
  EXPECT_EQ(ParseDate("2012-01-31"), Date(2012, 1, 31));
  EXPECT_EQ(ParseDate("2024-02-29"), Date(2024, 2, 29));  // leap year
}

TEST(DateTextTest, RejectsMalformedOrImpossibleDates) {
  for (const char* text : {"2026-02-30", "2025-02-29", "2026-1-05",
                           "18.10.2026", "2026-10-18x", "abcd-ef-gh", ""}) {
    const Result<std::chrono::year_month_day> date = ParseDate(text);
    ASSERT_FALSE(date.has_value()) << "accepted: " << text;
    EXPECT_EQ(date.error().code, ErrorCode::kInvalidArgument);
  }
}

}  // namespace
}  // namespace mymoney::core
