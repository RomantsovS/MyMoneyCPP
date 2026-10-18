#include "cli/arguments.h"

#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <string>
#include <string_view>

#include "core/error.h"

namespace mymoney::cli {
namespace {

constexpr std::string_view kOptionPrefix = "--";
constexpr std::string_view kHelpFlag = "help";

}  // namespace

core::Result<ParsedArguments> ParseArguments(
    std::span<const std::string_view> arguments) {
  ParsedArguments parsed;
  bool options_ended = false;

  for (auto it = arguments.begin(); it != arguments.end(); ++it) {
    const std::string_view argument = *it;
    if (options_ended || !argument.starts_with(kOptionPrefix)) {
      parsed.positionals.push_back(argument);
      continue;
    }
    if (argument == kOptionPrefix) {
      options_ended = true;
      continue;
    }

    std::string_view name = argument.substr(kOptionPrefix.size());
    std::string_view value;
    if (const std::size_t equals = name.find('=');
        equals != std::string_view::npos) {
      value = name.substr(equals + 1);
      name = name.substr(0, equals);
    } else if (name != kHelpFlag) {
      if (std::next(it) == arguments.end()) {
        return core::MakeError(core::ErrorCode::kInvalidArgument,
                               std::format("option --{} needs a value", name));
      }
      value = *++it;  // the next argument is the value
    }

    if (!parsed.options.emplace(std::string(name), std::string(value)).second) {
      return core::MakeError(core::ErrorCode::kInvalidArgument,
                             std::format("option --{} is given twice", name));
    }
  }
  return parsed;
}

}  // namespace mymoney::cli
