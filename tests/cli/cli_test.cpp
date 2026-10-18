// End-to-end tests of the CLI (ADR-0014): real commands, real services, a real
// SQLite database - in memory. Commands return text, so the tests compare it.

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "cli/application.h"
#include "cli/command_table.h"
#include "cli/commands/command_helpers.h"
#include "cli/context.h"
#include "core/test_util.h"
#include "storage/storage.h"

namespace mymoney::cli {
namespace {

class CliTest : public ::testing::Test {
 protected:
  RunResult Run(std::initializer_list<std::string_view> arguments) {
    const std::vector<std::string_view> list(arguments);
    return cli::Run(list, context_);
  }

  // Runs a command that must succeed and returns its output.
  std::string Ok(std::initializer_list<std::string_view> arguments) {
    const RunResult result = Run(arguments);
    EXPECT_EQ(result.exit_code, kExitOk) << result.error;
    return result.output;
  }

  void AddExampleData() {
    Ok({"currency", "add", "EUR", "--digits", "2"});
    Ok({"currency", "add", "USD", "--digits", "2"});
    Ok({"account", "add", "Card EUR", "--currency", "EUR"});
    Ok({"account", "add", "Cash EUR", "--currency", "EUR"});
    Ok({"account", "add", "Наличные USD", "--currency", "USD"});
    Ok({"category", "add", "Food", "--kind", "expense"});
    Ok({"category", "add", "Restaurants", "--kind", "expense", "--parent",
        "Food"});
    Ok({"category", "add", "Зарплата", "--kind", "income"});
  }

  Application application_{storage::OpenStorage(":memory:").value()};
  Context context_ = application_.MakeContext(core::test::Date(2026, 10, 18));
};

TEST_F(CliTest, RecordsAMonthAndShowsJournalAndBalances) {
  AddExampleData();
  EXPECT_EQ(
      Ok({"income", "Card EUR", "Зарплата", "3000", "--date", "2026-10-01"}),
      "Added income: 3000.00 EUR to 'Card EUR' (Зарплата) on 2026-10-01.\n");
  EXPECT_EQ(Ok({"transfer", "Card EUR", "Наличные USD", "100", "--amount-in",
                "108.50", "--date", "2026-10-03"}),
            "Added transfer: 100.00 EUR from 'Card EUR' to 'Наличные USD' "
            "(108.50 USD) on 2026-10-03.\n");
  // Without --date: the context's "today", 2026-10-18.
  EXPECT_EQ(
      Ok({"expense", "Card EUR", "Restaurants", "12,5", "--comment", "lunch"}),
      "Added expense: 12.50 EUR from 'Card EUR' (Restaurants) on "
      "2026-10-18.\n");

  // Columns: date, type, account(s), category, amount (right-aligned),
  // comment; two spaces between columns, widths from the widest cell.
  EXPECT_EQ(Ok({"journal"}),
            "2026-10-01  income    Card EUR                 "
            "Зарплата                 "
            "3000.00 EUR\n"
            "2026-10-03  transfer  Card EUR → Наличные USD               "
            "100.00 EUR → 108.50 USD\n"
            "2026-10-18  expense   Card EUR                 "
            "Restaurants               "
            "-12.50 EUR  lunch\n");
  EXPECT_EQ(Ok({"balance"}),
            "Card EUR      2887.50 EUR\n"
            "Cash EUR         0.00 EUR\n"
            "Наличные USD   108.50 USD\n"
            "Total EUR     2887.50 EUR\n"
            "Total USD      108.50 USD\n");
  EXPECT_EQ(Ok({"balance", "Card EUR", "--date", "2026-10-02"}),
            "Card EUR  3000.00 EUR\n");
}

TEST_F(CliTest, JournalFiltersByPeriodAndAccount) {
  AddExampleData();
  Ok({"expense", "Cash EUR", "Food", "5", "--date", "2026-09-30"});
  Ok({"expense", "Card EUR", "Food", "7", "--date", "2026-10-05"});

  EXPECT_EQ(Ok({"journal", "--from", "2026-10-01"}),
            "2026-10-05  expense  Card EUR  Food  -7.00 EUR\n");
  EXPECT_EQ(Ok({"journal", "--account", "Cash EUR"}),
            "2026-09-30  expense  Cash EUR  Food  -5.00 EUR\n");
  EXPECT_EQ(Ok({"journal", "--to", "2026-01-01"}), "No operations.\n");
}

TEST_F(CliTest, ListsCatalogData) {
  AddExampleData();
  Ok({"account", "archive", "Cash EUR"});

  EXPECT_EQ(Ok({"currency", "list"}),
            "EUR  2 decimal places\nUSD  2 decimal places\n");
  EXPECT_EQ(Ok({"account", "list"}),
            "Card EUR      EUR\n"
            "Cash EUR      EUR  archived\n"
            "Наличные USD  USD\n");
  EXPECT_EQ(Ok({"category", "list"}),
            "Food           expense\n"
            "  Restaurants  expense\n"
            "Зарплата       income\n");
}

TEST_F(CliTest, DomainErrorsExitWithOne) {
  AddExampleData();
  Ok({"account", "archive", "Cash EUR"});

  for (const std::initializer_list<std::string_view> arguments : {
           std::initializer_list<std::string_view>{"account", "add", "Card EUR",
                                                   "--currency", "EUR"},
           {"category", "add", "Food", "--kind", "income"},
           {"expense", "Card EUR", "Food", "12.505"},
           {"expense", "Card EUR", "Зарплата", "5"},
           {"expense", "Nowhere", "Food", "5"},
           {"expense", "Cash EUR", "Food", "5"},  // archived
           {"transfer", "Card EUR", "Наличные USD", "5"},
           {"expense", "Card EUR", "Food", "5", "--date", "2026-02-30"},
       }) {
    const RunResult result = Run(arguments);
    EXPECT_EQ(result.exit_code, kExitError) << *arguments.begin();
    EXPECT_TRUE(result.error.starts_with("error: ")) << result.error;
    EXPECT_TRUE(result.output.empty());
  }
}

TEST_F(CliTest, UsageErrorsExitWithTwo) {
  for (const std::initializer_list<std::string_view> arguments : {
           std::initializer_list<std::string_view>{},
           {"frobnicate"},
           {"journal", "--color", "red"},
           {"account", "add", "Cash"},  // --currency is required
           {"expense", "Cash", "Food"},
           {"journal", "--from"},
       }) {
    const RunResult result = Run(arguments);
    EXPECT_EQ(result.exit_code, kExitUsage) << result.error;
    EXPECT_TRUE(result.error.starts_with("error: ")) << result.error;
  }
}

TEST_F(CliTest, HelpListsEveryCommand) {
  const std::string help = Ok({"--help"});

  for (const CommandSpec& command : Commands()) {
    EXPECT_NE(help.find(command.name), std::string::npos) << command.name;
  }
  EXPECT_EQ(Ok({"transfer", "--help"}).find("Usage: mymoney-cli transfer"), 0U);
}

TEST(TextTableTest, AlignsByDisplayWidthNotBytes) {
  commands::TextTable table(
      {commands::TextTable::Align::kLeft, commands::TextTable::Align::kRight});
  table.AddRow({"Ab", "1.00"});
  table.AddRow({"Ящик", "10.00"});  // 4 letters, 8 bytes

  EXPECT_EQ(table.Render(), "Ab     1.00\nЯщик  10.00\n");
  EXPECT_EQ(commands::DisplayWidth("Ящик"), 4U);
}

}  // namespace
}  // namespace mymoney::cli
