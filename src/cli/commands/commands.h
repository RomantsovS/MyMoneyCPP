#ifndef MYMONEY_CLI_COMMANDS_COMMANDS_H_
#define MYMONEY_CLI_COMMANDS_COMMANDS_H_

// The command handlers, referenced by the command table. Each one checks its
// input, calls services or repositories and returns its output text.

#include <string>

#include "cli/command_table.h"
#include "cli/context.h"
#include "core/error.h"

namespace mymoney::cli::commands {

core::Result<std::string> CurrencyAdd(Context& context,
                                      const CommandCall& call);
core::Result<std::string> CurrencyList(Context& context,
                                       const CommandCall& call);

core::Result<std::string> AccountAdd(Context& context, const CommandCall& call);
core::Result<std::string> AccountList(Context& context,
                                      const CommandCall& call);
core::Result<std::string> AccountArchive(Context& context,
                                         const CommandCall& call);

core::Result<std::string> CategoryAdd(Context& context,
                                      const CommandCall& call);
core::Result<std::string> CategoryList(Context& context,
                                       const CommandCall& call);

core::Result<std::string> Expense(Context& context, const CommandCall& call);
core::Result<std::string> Income(Context& context, const CommandCall& call);
core::Result<std::string> Transfer(Context& context, const CommandCall& call);

core::Result<std::string> Journal(Context& context, const CommandCall& call);
core::Result<std::string> Balance(Context& context, const CommandCall& call);

}  // namespace mymoney::cli::commands

#endif  // MYMONEY_CLI_COMMANDS_COMMANDS_H_
