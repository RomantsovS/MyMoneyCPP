#ifndef MYMONEY_CLI_ENVIRONMENT_H_
#define MYMONEY_CLI_ENVIRONMENT_H_

// What the CLI needs from the operating system: today's date and the default
// database location. Kept apart from the commands, which get both injected.

#include <chrono>
#include <filesystem>

#include "core/error.h"

namespace mymoney::cli {

// Today's date in the local time zone.
[[nodiscard]] std::chrono::year_month_day LocalToday();

// <per-user data folder>/MyMoney/mymoney.sqlite (ADR-0014):
//   Windows: %LOCALAPPDATA%;  Linux: $XDG_DATA_HOME or ~/.local/share.
// kNotFound if the needed environment variables are not set.
[[nodiscard]] core::Result<std::filesystem::path> DefaultDatabasePath();

}  // namespace mymoney::cli

#endif  // MYMONEY_CLI_ENVIRONMENT_H_
