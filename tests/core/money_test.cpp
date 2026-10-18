#include "core/money.h"

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "core/test_util.h"

namespace mymoney::core {
namespace {

using test::Eur;
using test::Usd;

// Money is usable in constant expressions.
static_assert(Eur(150) + Eur(50) == Eur(200));
static_assert(-Eur(30) == Eur(-30));

TEST(MoneyTest, AddsAndSubtractsInOneCurrency) {
  EXPECT_EQ(Eur(1050) + Eur(250), Eur(1300));
  EXPECT_EQ(Eur(1050) - Eur(2000), Eur(-950));

  Money total = Money::Zero(test::kEur);
  total += Eur(100);
  total -= Eur(30);
  EXPECT_EQ(total, Eur(70));
}

TEST(MoneyTest, KnowsItsSign) {
  EXPECT_TRUE(Eur(0).IsZero());
  EXPECT_TRUE(Eur(1).IsPositive());
  EXPECT_TRUE(Eur(-1).IsNegative());
  EXPECT_EQ(-Eur(500), Eur(-500));
}

TEST(MoneyTest, ComparesWithinOneCurrency) {
  EXPECT_LT(Eur(100), Eur(200));
  EXPECT_GT(Eur(0), Eur(-1));
}

TEST(MoneyTest, AmountsInDifferentCurrenciesAreNotEqual) {
  EXPECT_NE(Eur(100), Usd(100));
}

// Breaking a precondition is a programming error: the program stops with a
// message. A death test runs the statement in a child process and checks that
// it dies with output matching the regular expression.
TEST(MoneyDeathTest, AddingDifferentCurrenciesStopsTheProgram) {
  EXPECT_DEATH((void)(Eur(100) + Usd(100)), "same currency");
}

TEST(MoneyDeathTest, ComparingDifferentCurrenciesStopsTheProgram) {
  EXPECT_DEATH((void)(Eur(100) < Usd(100)), "same currency");
}

TEST(MoneyDeathTest, OverflowStopsTheProgram) {
  const Money huge(std::numeric_limits<std::int64_t>::max(), test::kEur);

  EXPECT_DEATH((void)(huge + Eur(1)), "overflows");
  EXPECT_DEATH((void)(-huge - Eur(2)), "overflows");
}

}  // namespace
}  // namespace mymoney::core
