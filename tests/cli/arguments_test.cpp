#include "cli/arguments.h"

#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "core/error.h"

namespace mymoney::cli {
namespace {

core::Result<ParsedArguments> Parse(std::vector<std::string_view> arguments) {
  return ParseArguments(arguments);
}

TEST(ParseArgumentsTest, SplitsPositionalsAndOptions) {
  const auto parsed = Parse({"expense", "Cash EUR", "--date", "2026-10-18",
                             "Food", "--comment=a=b", "12.50"});

  ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
  EXPECT_EQ(parsed->positionals, (std::vector<std::string_view>{
                                     "expense", "Cash EUR", "Food", "12.50"}));
  EXPECT_EQ(parsed->options,
            (Options{{"date", "2026-10-18"}, {"comment", "a=b"}}));
}

TEST(ParseArgumentsTest, HelpIsAFlagWithoutValue) {
  const auto parsed = Parse({"--help", "journal"});

  ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
  EXPECT_EQ(parsed->positionals, (std::vector<std::string_view>{"journal"}));
  EXPECT_EQ(parsed->options, (Options{{"help", ""}}));
}

TEST(ParseArgumentsTest, DoubleDashEndsOptions) {
  const auto parsed = Parse({"account", "add", "--", "--strange name"});

  ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
  EXPECT_EQ(parsed->positionals.back(), "--strange name");
  EXPECT_TRUE(parsed->options.empty());
}

TEST(ParseArgumentsTest, SingleDashIsAPositional) {
  // So that "-5" reaches the amount parser, which explains the problem.
  const auto parsed = Parse({"expense", "-5"});

  ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
  EXPECT_EQ(parsed->positionals.back(), "-5");
}

TEST(ParseArgumentsTest, RejectsOptionWithoutValue) {
  const auto parsed = Parse({"journal", "--from"});

  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(parsed.error().code, core::ErrorCode::kInvalidArgument);
}

TEST(ParseArgumentsTest, RejectsRepeatedOption) {
  const auto parsed =
      Parse({"journal", "--from", "2026-01-01", "--from=2026-02-01"});

  ASSERT_FALSE(parsed.has_value());
  EXPECT_EQ(parsed.error().code, core::ErrorCode::kInvalidArgument);
}

}  // namespace
}  // namespace mymoney::cli
