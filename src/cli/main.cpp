// mymoney-cli: command-line interface (ADR-0010, ADR-0014).
//
// The composition root: reads the command line, opens the database, wires the
// application together, runs one command and prints its result. Everything
// else lives in mymoney_cli_lib, where it is tested.

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <format>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "cli/application.h"
#include "cli/arguments.h"
#include "cli/command_table.h"
#include "cli/environment.h"
#include "core/check.h"
#include "core/error.h"
#include "storage/sqlite/database.h"
#include "storage/storage.h"

namespace {

using mymoney::cli::kExitError;
using mymoney::cli::kExitUsage;
using mymoney::cli::RunResult;

int Print(const RunResult& result) {
  if (!result.output.empty()) {
    std::print("{}", result.output);
  }
  if (!result.error.empty()) {
    std::print(stderr, "{}", result.error);
  }
  return result.exit_code;
}

int Fail(std::string_view message) {
  std::print(stderr, "error: {}\n", message);
  return kExitError;
}

// --db FILE, or the default location (whose folder is created if needed).
mymoney::core::Result<std::string> DatabasePath(
    const mymoney::cli::ParsedArguments& parsed) {
  if (const auto it = parsed.options.find("db"); it != parsed.options.end()) {
    return it->second;
  }
  return mymoney::cli::DefaultDatabasePath().and_then(
      [](const std::filesystem::path& path)
          -> mymoney::core::Result<std::string> {
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
          return mymoney::core::MakeError(
              mymoney::core::ErrorCode::kStorage,
              std::format("cannot create folder for the database: {}",
                          error.message()));
        }
        // SQLite expects UTF-8 file names.
        const std::u8string utf8 = path.u8string();
        return std::string(utf8.begin(), utf8.end());
      });
}

}  // namespace

int main(int argc, char* argv[]) {
  // The arguments without the program name; they live until main returns.
  const std::vector<std::string_view> arguments(
      std::from_range,
      std::span(argv, static_cast<std::size_t>(argc)) | std::views::drop(1));

  const auto parsed = mymoney::cli::ParseArguments(arguments);
  if (!parsed) {
    return Print({.exit_code = kExitUsage,
                  .output = {},
                  .error = std::format("error: {}\n", parsed.error().message)});
  }

  // --help and wrong usage are answered without touching the database.
  const auto prepared = mymoney::cli::Prepare(*parsed);
  if (const auto* finished = std::get_if<RunResult>(&prepared)) {
    return Print(*finished);
  }

  const auto path = DatabasePath(*parsed);
  if (!path) {
    return Fail(path.error().message);
  }
  auto db = mymoney::storage::OpenStorage(*path);
  if (!db) {
    return Fail(db.error().message);
  }

  mymoney::cli::Application application(std::move(*db));
  mymoney::cli::Context context =
      application.MakeContext(mymoney::cli::LocalToday());
  // Prepare() returned no finished result, so it holds the matched command.
  const auto* command = std::get_if<mymoney::cli::MatchedCommand>(&prepared);
  mymoney::core::Check(command != nullptr, "Prepare() returned nothing");
  return Print(mymoney::cli::Execute(*command, context));
}
