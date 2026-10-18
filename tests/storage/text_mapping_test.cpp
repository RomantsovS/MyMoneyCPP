#include "storage/text_mapping.h"

#include <chrono>

#include <gtest/gtest.h>

#include "core/category.h"
#include "core/error.h"
#include "core/operation.h"
#include "core/test_util.h"

namespace mymoney::storage {
namespace {

using core::test::Date;

TEST(TextMappingTest, DatesAreIsoText) {
  EXPECT_EQ(DateToText(Date(2026, 10, 8)), "2026-10-08");
  EXPECT_EQ(DateFromText("2012-01-31"), Date(2012, 1, 31));
}

TEST(TextMappingTest, RejectsMalformedOrImpossibleDates) {
  for (const char* text : {"2026-02-30", "2026-1-05", "18.10.2026",
                           "2026-10-18x", "abcd-ef-gh", ""}) {
    const core::Result<std::chrono::year_month_day> date = DateFromText(text);
    ASSERT_FALSE(date.has_value()) << "accepted: " << text;
    EXPECT_EQ(date.error().code, core::ErrorCode::kStorage);
  }
}

TEST(TextMappingTest, EnumsRoundTrip) {
  for (const core::CategoryKind kind :
       {core::CategoryKind::kExpense, core::CategoryKind::kIncome}) {
    EXPECT_EQ(CategoryKindFromText(CategoryKindToText(kind)), kind);
  }
  for (const core::OperationType type :
       {core::OperationType::kExpense, core::OperationType::kIncome,
        core::OperationType::kTransfer}) {
    EXPECT_EQ(OperationTypeFromText(OperationTypeToText(type)), type);
  }
  EXPECT_FALSE(OperationTypeFromText("Expense").has_value());
}

}  // namespace
}  // namespace mymoney::storage
