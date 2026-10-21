#include "ui/money_format.h"

#include <cstdint>
#include <limits>

#include <QLocale>
#include <QString>

#include <gtest/gtest.h>

#include "core/currency_code.h"
#include "core/money.h"
#include "core/test_util.h"
#include "ui/qt_print.h"

namespace mymoney::ui {
namespace {

// u"..."_s is a QString.
using Qt::Literals::StringLiterals::operator""_s;

using core::test::Eur;

// Functions, not globals: a QLocale constructor may throw, and an exception
// during static initialization could not be caught.
QLocale English() { return {QLocale::English, QLocale::UnitedStates}; }
QLocale Russian() { return {QLocale::Russian, QLocale::Russia}; }

TEST(MoneyFormatTest, UsesTheLocalesSeparators) {
  EXPECT_EQ(FormatMoney(Eur(123450), 2, English()), u"1,234.50 EUR"_s);
  // Russian groups with a (no-break) space and uses a decimal comma.
  EXPECT_EQ(FormatMoney(Eur(123450), 2, Russian()),
            u"1"_s + Russian().groupSeparator() + u"234,50 EUR"_s);
  // The C locale does not group digits.
  EXPECT_EQ(FormatMoney(Eur(123450), 2, QLocale::c()), u"1234.50 EUR"_s);
}

TEST(MoneyFormatTest, PadsTheFractionAndKeepsTheSign) {
  EXPECT_EQ(FormatMoney(Eur(-5), 2, English()), u"-0.05 EUR"_s);
  EXPECT_EQ(FormatMoney(Eur(0), 2, English()), u"0.00 EUR"_s);
  // Four decimal places: the fraction is never grouped (not "2,345").
  EXPECT_EQ(FormatMoney(Eur(12345), 4, English()), u"1.2345 EUR"_s);
}

TEST(MoneyFormatTest, CurrencyWithoutDecimals) {
  const core::Money yen(1234, core::CurrencyCode("JPY"));

  EXPECT_EQ(FormatMoney(yen, 0, English()), u"1,234 JPY"_s);
}

TEST(MoneyFormatTest, ExactForTheLargestAmounts) {
  // Through a double these would lose their last digits.
  EXPECT_EQ(FormatMoney(Eur(1'000'000'000'000'001), 2, English()),
            u"10,000,000,000,000.01 EUR"_s);
  EXPECT_EQ(
      FormatMoney(Eur(std::numeric_limits<std::int64_t>::min()), 2, English()),
      u"-92,233,720,368,547,758.08 EUR"_s);
}

}  // namespace
}  // namespace mymoney::ui
