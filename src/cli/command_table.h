#ifndef MYMONEY_CLI_COMMAND_TABLE_H_
#define MYMONEY_CLI_COMMAND_TABLE_H_

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cli/arguments.h"
#include "cli/context.h"
#include "core/error.h"

namespace mymoney::cli {

inline constexpr int kExitOk = 0;
inline constexpr int kExitError = 1;  // an error from the domain or storage
inline constexpr int kExitUsage = 2;  // the command line itself is wrong

// What a command handler receives besides the context.
struct CommandCall {
  std::vector<std::string_view>
      arguments;                     // positionals after the command name
  const Options* options = nullptr;  // never null once prepared

  // The value of --name, if given.
  [[nodiscard]] std::optional<std::string_view> Option(
      std::string_view name) const;
};

// A command returns its output as text; only main prints (ADR-0014).
using Handler = core::Result<std::string> (*)(Context& context,
                                              const CommandCall& call);

// One row of the command table.
struct CommandSpec {
  std::string_view name;         // one or two words: "balance", "account add"
  std::string_view usage;        // "<name> --currency CODE"
  std::string_view description;  // one line for --help
  std::size_t min_arguments;
  std::size_t max_arguments;
  std::span<const std::string_view> options;           // allowed (no "--")
  std::span<const std::string_view> required_options;  // subset of options
  Handler handler;
};

// All commands, in the order --help lists them.
[[nodiscard]] std::span<const CommandSpec> Commands();

// The overview printed by --help.
[[nodiscard]] std::string HelpText();

struct MatchedCommand {
  const CommandSpec* spec = nullptr;  // never null once prepared
  CommandCall call;
};

struct RunResult {
  int exit_code;
  std::string output;  // for stdout
  std::string error;   // for stderr
};

// Everything that needs no database: --help, unknown commands, wrong number
// of arguments, unknown or missing options. Returns either a finished result
// (to print) or the command to execute. `parsed` must outlive the result.
[[nodiscard]] std::variant<RunResult, MatchedCommand> Prepare(
    const ParsedArguments& parsed);

// Runs a prepared command against the context.
[[nodiscard]] RunResult Execute(const MatchedCommand& command,
                                Context& context);

// Parse + Prepare + Execute in one step (used by tests). Ignores --db: the
// context already decides which database is used.
[[nodiscard]] RunResult Run(std::span<const std::string_view> arguments,
                            Context& context);

}  // namespace mymoney::cli

#endif  // MYMONEY_CLI_COMMAND_TABLE_H_
