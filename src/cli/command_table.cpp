#include "cli/command_table.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cli/arguments.h"
#include "cli/commands/commands.h"
#include "cli/context.h"
#include "core/error.h"

namespace mymoney::cli {
namespace {

constexpr std::string_view kProgram = "mymoney-cli";

// Accepted by every command: --help shows its usage, --db is used by main.
constexpr std::array<std::string_view, 2> kGlobalOptions{"help", "db"};

constexpr std::array<std::string_view, 0> kNone{};
constexpr std::array<std::string_view, 1> kDigits{"digits"};
constexpr std::array<std::string_view, 1> kCurrency{"currency"};
constexpr std::array<std::string_view, 2> kCategoryOptions{"group", "kind"};
constexpr std::array<std::string_view, 1> kKind{"kind"};
constexpr std::array<std::string_view, 2> kOperationOptions{"date", "comment"};
constexpr std::array<std::string_view, 3> kTransferOptions{"date", "comment",
                                                           "amount-in"};
constexpr std::array<std::string_view, 3> kJournalOptions{"from", "to",
                                                          "account"};
constexpr std::array<std::string_view, 1> kDate{"date"};

// clang-format off
const std::array<CommandSpec, 13> kCommands{{
    {.name = "currency add", .usage = "<CODE> --digits N",
     .description = "Add a currency, e.g. EUR with 2 decimal places",
     .min_arguments = 1, .max_arguments = 1,
     .options = kDigits, .required_options = kDigits,
     .handler = commands::CurrencyAdd},
    {.name = "currency list", .usage = "",
     .description = "List the currencies",
     .min_arguments = 0, .max_arguments = 0,
     .options = kNone, .required_options = kNone,
     .handler = commands::CurrencyList},
    {.name = "account add", .usage = "<name> --currency CODE",
     .description = "Add an account",
     .min_arguments = 1, .max_arguments = 1,
     .options = kCurrency, .required_options = kCurrency,
     .handler = commands::AccountAdd},
    {.name = "account list", .usage = "",
     .description = "List the accounts",
     .min_arguments = 0, .max_arguments = 0,
     .options = kNone, .required_options = kNone,
     .handler = commands::AccountList},
    {.name = "account archive", .usage = "<name>",
     .description = "Archive an account: it keeps its history, gets no new operations",
     .min_arguments = 1, .max_arguments = 1,
     .options = kNone, .required_options = kNone,
     .handler = commands::AccountArchive},
    {.name = "group add", .usage = "<name> --kind expense|income",
     .description = "Add a group of categories (always at the root, has no operations)",
     .min_arguments = 1, .max_arguments = 1,
     .options = kKind, .required_options = kKind,
     .handler = commands::GroupAdd},
    {.name = "category add", .usage = "<name> [--group NAME] [--kind expense|income]",
     .description = "Add a category, at the root or in a group (--kind: taken from the group)",
     .min_arguments = 1, .max_arguments = 1,
     .options = kCategoryOptions, .required_options = kNone,
     .handler = commands::CategoryAdd},
    {.name = "category list", .usage = "",
     .description = "Show the category tree",
     .min_arguments = 0, .max_arguments = 0,
     .options = kNone, .required_options = kNone,
     .handler = commands::CategoryList},
    {.name = "expense", .usage = "<account> <category> <amount> [--date D] [--comment TEXT]",
     .description = "Record money spent",
     .min_arguments = 3, .max_arguments = 3,
     .options = kOperationOptions, .required_options = kNone,
     .handler = commands::Expense},
    {.name = "income", .usage = "<account> <category> <amount> [--date D] [--comment TEXT]",
     .description = "Record money received",
     .min_arguments = 3, .max_arguments = 3,
     .options = kOperationOptions, .required_options = kNone,
     .handler = commands::Income},
    {.name = "transfer",
     .usage = "<from> <to> <amount> [--amount-in AMOUNT] [--date D] [--comment TEXT]",
     .description = "Move money between accounts (--amount-in: between currencies)",
     .min_arguments = 3, .max_arguments = 3,
     .options = kTransferOptions, .required_options = kNone,
     .handler = commands::Transfer},
    {.name = "journal", .usage = "[--from D] [--to D] [--account NAME]",
     .description = "List operations, oldest first",
     .min_arguments = 0, .max_arguments = 0,
     .options = kJournalOptions, .required_options = kNone,
     .handler = commands::Journal},
    {.name = "balance", .usage = "[<account>] [--date D]",
     .description = "Show balances (at the end of day D, default: now)",
     .min_arguments = 0, .max_arguments = 1,
     .options = kDate, .required_options = kNone,
     .handler = commands::Balance},
}};
// clang-format on

std::string CommandUsage(const CommandSpec& spec) {
  return std::format("Usage: {} {} {}", kProgram, spec.name, spec.usage);
}

RunResult UsageError(std::string_view message, std::string_view hint) {
  return {.exit_code = kExitUsage,
          .output = {},
          .error = std::format("error: {}\n{}\n", message, hint)};
}

std::string GeneralHint() {
  return std::format("Run '{} --help' for the list of commands.", kProgram);
}

bool Contains(std::span<const std::string_view> list, std::string_view value) {
  return std::ranges::find(list, value) != list.end();
}

// The command whose name matches the first one or two positionals.
const CommandSpec* FindCommand(std::span<const std::string_view> words,
                               std::size_t* name_words) {
  for (const std::size_t count : {std::size_t{2}, std::size_t{1}}) {
    if (words.size() < count) {
      continue;
    }
    const std::string name = count == 2 ? std::format("{} {}", words.front(),
                                                      *std::next(words.begin()))
                                        : std::string(words.front());
    const auto it = std::ranges::find(kCommands, name, &CommandSpec::name);
    if (it != kCommands.end()) {
      *name_words = count;
      return &*it;
    }
  }
  return nullptr;
}

}  // namespace

std::optional<std::string_view> CommandCall::Option(
    std::string_view name) const {
  const auto it = options->find(name);
  if (it == options->end()) {
    return std::nullopt;
  }
  return it->second;
}

std::span<const CommandSpec> Commands() { return kCommands; }

std::string HelpText() {
  std::string text = std::format(
      "{} - MyMoney on the command line\n\n"
      "Usage: {} [--db FILE] <command> [arguments] [options]\n\n"
      "Commands:\n",
      kProgram, kProgram);
  for (const CommandSpec& spec : kCommands) {
    text += std::format("  {} {}\n      {}\n", spec.name, spec.usage,
                        spec.description);
  }
  text +=
      "\nAccounts, groups and categories are given by name; quote names with "
      "spaces.\n"
      "Amounts: 12.50 or 12,50. Dates: YYYY-MM-DD (default: today).\n"
      "--db FILE: the database to use (default: your user data folder).\n"
      "<command> --help shows the usage of one command.\n";
  return text;
}

std::variant<RunResult, MatchedCommand> Prepare(const ParsedArguments& parsed) {
  const bool help = parsed.options.contains("help");
  if (parsed.positionals.empty()) {
    if (help) {
      return RunResult{.exit_code = kExitOk, .output = HelpText(), .error = {}};
    }
    return UsageError("no command given", GeneralHint());
  }

  std::size_t name_words = 0;
  const CommandSpec* spec = FindCommand(parsed.positionals, &name_words);
  if (spec == nullptr) {
    return UsageError(
        std::format("unknown command '{}'", parsed.positionals.front()),
        GeneralHint());
  }
  if (help) {
    return RunResult{.exit_code = kExitOk,
                     .output = std::format("{}\n{}\n", CommandUsage(*spec),
                                           spec->description),
                     .error = {}};
  }

  std::vector<std::string_view> arguments(
      std::next(parsed.positionals.begin(),
                static_cast<std::ptrdiff_t>(name_words)),
      parsed.positionals.end());
  if (arguments.size() < spec->min_arguments ||
      arguments.size() > spec->max_arguments) {
    return UsageError(
        std::format("wrong number of arguments for '{}'", spec->name),
        CommandUsage(*spec));
  }
  for (const auto& [name, value] : parsed.options) {
    if (!Contains(spec->options, name) && !Contains(kGlobalOptions, name)) {
      return UsageError(
          std::format("'{}' has no option --{}", spec->name, name),
          CommandUsage(*spec));
    }
  }
  for (const std::string_view required : spec->required_options) {
    if (!parsed.options.contains(required)) {
      return UsageError(
          std::format("'{}' needs the option --{}", spec->name, required),
          CommandUsage(*spec));
    }
  }
  return MatchedCommand{
      .spec = spec,
      .call = {.arguments = std::move(arguments), .options = &parsed.options}};
}

RunResult Execute(const MatchedCommand& command, Context& context) {
  core::Result<std::string> output =
      command.spec->handler(context, command.call);
  if (!output) {
    return {.exit_code = kExitError,
            .output = {},
            .error = std::format("error: {}\n", output.error().message)};
  }
  return {.exit_code = kExitOk, .output = std::move(*output), .error = {}};
}

RunResult Run(std::span<const std::string_view> arguments, Context& context) {
  const core::Result<ParsedArguments> parsed = ParseArguments(arguments);
  if (!parsed) {
    return UsageError(parsed.error().message, GeneralHint());
  }
  std::variant<RunResult, MatchedCommand> prepared = Prepare(*parsed);
  if (RunResult* finished = std::get_if<RunResult>(&prepared)) {
    return std::move(*finished);
  }
  return Execute(std::get<MatchedCommand>(prepared), context);
}

}  // namespace mymoney::cli
