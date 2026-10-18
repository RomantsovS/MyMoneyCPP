#ifndef MYMONEY_CLI_COMMANDS_COMMAND_HELPERS_H_
#define MYMONEY_CLI_COMMANDS_COMMAND_HELPERS_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cli/command_table.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/money.h"
#include "core/repositories/currency_repository.h"

namespace mymoney::cli::commands {

// Money <-> text for the CLI: "12.50 EUR". Looks up each currency's number of
// decimals once and remembers it (a journal shows many amounts).
class MoneyText {
 public:
  // The repository is not owned and must outlive this object.
  explicit MoneyText(core::CurrencyRepository* currencies);

  [[nodiscard]] core::Result<core::Money> Parse(std::string_view text,
                                                core::CurrencyCode currency);
  [[nodiscard]] core::Result<std::string> Format(const core::Money& money);

 private:
  [[nodiscard]] core::Result<int> Digits(core::CurrencyCode currency);

  core::CurrencyRepository* currencies_;
  std::map<core::CurrencyCode, int> digits_;
};

// The date given with --<name>, or `fallback` if the option is missing.
[[nodiscard]] core::Result<std::chrono::year_month_day> DateOption(
    const CommandCall& call, std::string_view name,
    std::chrono::year_month_day fallback);

// The date given with --<name>, or nullopt if the option is missing.
[[nodiscard]] core::Result<std::optional<std::chrono::year_month_day>>
OptionalDateOption(const CommandCall& call, std::string_view name);

// How many characters `text` shows on screen: UTF-8 code points, so a
// Cyrillic name counts as many letters as it has, not twice as many bytes.
[[nodiscard]] std::size_t DisplayWidth(std::string_view text);

// A plain text table: columns separated by two spaces, aligned by display
// width, no trailing spaces.
class TextTable {
 public:
  enum class Align : std::uint8_t { kLeft, kRight };

  explicit TextTable(std::vector<Align> alignment);

  void AddRow(std::vector<std::string> cells);
  [[nodiscard]] std::string Render() const;

 private:
  std::vector<Align> alignment_;
  std::vector<std::vector<std::string>> rows_;
};

}  // namespace mymoney::cli::commands

#endif  // MYMONEY_CLI_COMMANDS_COMMAND_HELPERS_H_
