#include "cli/environment.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <optional>
#include <string>

#include "core/error.h"

namespace mymoney::cli {
namespace {

std::optional<std::string> GetEnvironment(const char* name) {
#ifdef _MSC_VER
#pragma warning(push)
// MSVC suggests _dupenv_s; getenv is fine here: we only read, single-threaded.
#pragma warning(disable : 4996)
#endif
  const char* value = std::getenv(name);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  if (value == nullptr || *value == '\0') {
    return std::nullopt;
  }
  return std::string(value);
}

}  // namespace

std::chrono::year_month_day LocalToday() {
  const std::time_t now = std::time(nullptr);
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  return {std::chrono::year{local.tm_year + 1900},
          std::chrono::month{static_cast<unsigned>(local.tm_mon + 1)},
          std::chrono::day{static_cast<unsigned>(local.tm_mday)}};
}

core::Result<std::filesystem::path> DefaultDatabasePath() {
#ifdef _WIN32
  const std::optional<std::string> base = GetEnvironment("LOCALAPPDATA");
#else
  std::optional<std::string> base = GetEnvironment("XDG_DATA_HOME");
  if (!base.has_value()) {
    base = GetEnvironment("HOME").transform(
        [](const std::string& home) { return home + "/.local/share"; });
  }
#endif
  if (!base.has_value()) {
    return core::MakeError(core::ErrorCode::kNotFound,
                           "cannot find the user data folder; use --db FILE");
  }
  return std::filesystem::path(*base) / "MyMoney" / "mymoney.sqlite";
}

}  // namespace mymoney::cli
