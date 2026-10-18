#ifndef MYMONEY_CLI_ARGUMENTS_H_
#define MYMONEY_CLI_ARGUMENTS_H_

#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/error.h"

namespace mymoney::cli {

// Option name (without "--") -> value; flags such as --help have "".
// std::less<> allows lookups with a std::string_view.
using Options = std::map<std::string, std::string, std::less<>>;

// The command line split into positionals and options (ADR-0014). The
// positionals point into the original argument strings.
struct ParsedArguments {
  std::vector<std::string_view> positionals;
  Options options;
};

// Splits arguments (without the program name):
//   positional        -> positionals
//   --name value      -> options["name"] = "value"
//   --name=value      -> options["name"] = "value"
//   --help            -> options["help"] = ""  (the only flag)
//   --                -> everything after it is positional
// kInvalidArgument for an option without a value or a repeated option.
[[nodiscard]] core::Result<ParsedArguments> ParseArguments(
    std::span<const std::string_view> arguments);

}  // namespace mymoney::cli

#endif  // MYMONEY_CLI_ARGUMENTS_H_
