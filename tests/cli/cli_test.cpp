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
    Ok({"group", "add", "Food", "--kind", "expense"});
    Ok({"category", "add", "Groceries", "--group", "Food"});
    Ok({"category", "add", "Restaurants", "--group", "Food"});
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
  Ok({"expense", "Cash EUR", "Groceries", "5", "--date", "2026-09-30"});
  Ok({"expense", "Card EUR", "Groceries", "7", "--date", "2026-10-05"});

  EXPECT_EQ(Ok({"journal", "--from", "2026-10-01"}),
            "2026-10-05  expense  Card EUR  Groceries  -7.00 EUR\n");
  EXPECT_EQ(Ok({"journal", "--account", "Cash EUR"}),
            "2026-09-30  expense  Cash EUR  Groceries  -5.00 EUR\n");
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
  // Groups are marked; an empty group is listed too.
  Ok({"group", "add", "Подарки", "--kind", "income"});
  EXPECT_EQ(Ok({"category", "list"}),
            "Food           expense group\n"
            "  Groceries    expense\n"
            "  Restaurants  expense\n"
            "Зарплата       income\n"
            "Подарки        income group\n");
}

TEST_F(CliTest, AddsGroupsAndCategories) {
  Ok({"currency", "add", "EUR", "--digits", "2"});

  EXPECT_EQ(Ok({"group", "add", "Food", "--kind", "expense"}),
            "Added expense group 'Food'.\n");
  // The kind comes from the group; giving the same one again is fine.
  EXPECT_EQ(Ok({"category", "add", "Groceries", "--group", "Food"}),
            "Added expense category 'Groceries' in group 'Food'.\n");
  EXPECT_EQ(
      Ok({"category", "add", "Snacks", "--group", "Food", "--kind", "expense"}),
      "Added expense category 'Snacks' in group 'Food'.\n");
  EXPECT_EQ(Ok({"category", "add", "Transport", "--kind", "expense"}),
            "Added expense category 'Transport'.\n");
}

TEST_F(CliTest, OperationOnAGroupNamesItsCategories) {
  AddExampleData();
  Ok({"group", "add", "Home", "--kind", "expense"});

  RunResult result = Run({"expense", "Card EUR", "Food", "5"});
  EXPECT_EQ(result.exit_code, kExitError);
  EXPECT_EQ(result.error,
            "error: 'Food' is a group; use one of its categories: Groceries, "
            "Restaurants\n");

  result = Run({"expense", "Card EUR", "Home", "5"});
  EXPECT_EQ(result.exit_code, kExitError);
  EXPECT_EQ(result.error,
            "error: 'Home' is a group without categories; add one with "
            "'category add <name> --group \"Home\"'\n");
}

TEST_F(CliTest, DomainErrorsExitWithOne) {
  AddExampleData();
  Ok({"account", "archive", "Cash EUR"});
  Ok({"category", "add", "Transport", "--kind", "expense"});

  for (const std::initializer_list<std::string_view> arguments : {
           std::initializer_list<std::string_view>{"account", "add", "Card EUR",
                                                   "--currency", "EUR"},
           {"category", "add", "Food", "--kind", "income"},
           {"group", "add", "Groceries", "--kind", "expense"},
           {"group", "add", "Bills", "--kind", "spending"},
           {"category", "add", "Taxi"},  // no --kind and no --group
           {"category", "add", "Taxi", "--group", "Transport"},  // a category
           {"category", "add", "Taxi", "--group", "Nowhere"},
           {"category", "add", "Bonus", "--group", "Food", "--kind", "income"},
           {"expense", "Card EUR", "Groceries", "12.505"},
           {"expense", "Card EUR", "Зарплата", "5"},
           {"expense", "Card EUR", "Food", "5"},  // a group
           {"expense", "Nowhere", "Groceries", "5"},
           {"expense", "Cash EUR", "Groceries", "5"},  // archived
           {"transfer", "Card EUR", "Наличные USD", "5"},
           {"expense", "Card EUR", "Groceries", "5", "--date", "2026-02-30"},
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
           {"group", "add", "Bills"},   // --kind is required
           {"category", "add", "Taxi", "--parent", "Food"},  // was M3 syntax
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
