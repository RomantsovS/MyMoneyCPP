#include "cli/commands/command_helpers.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cli/command_table.h"
#include "core/amount_text.h"
#include "core/check.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/date_text.h"
#include "core/error.h"
#include "core/money.h"
#include "core/repositories/currency_repository.h"

namespace mymoney::cli::commands {

MoneyText::MoneyText(core::CurrencyRepository* currencies)
    : currencies_(currencies) {
  core::Check(currencies_ != nullptr, "MoneyText needs a CurrencyRepository");
}

core::Result<int> MoneyText::Digits(core::CurrencyCode currency) {
  if (const auto it = digits_.find(currency); it != digits_.end()) {
    return it->second;
  }
  return currencies_->Get(currency).transform([&](const core::Currency& known) {
    digits_.emplace(currency, known.minor_digits());
    return known.minor_digits();
  });
}

core::Result<core::Money> MoneyText::Parse(std::string_view text,
                                           core::CurrencyCode currency) {
  return Digits(currency)
      .and_then([&](int digits) { return core::ParseAmount(text, digits); })
      .transform([&](std::int64_t minor_units) {
        return core::Money(minor_units, currency);
      });
}

core::Result<std::string> MoneyText::Format(const core::Money& money) {
  return Digits(money.currency()).transform([&](int digits) {
    return std::format("{} {}", core::FormatAmount(money.minor_units(), digits),
                       money.currency().code());
  });
}

core::Result<std::chrono::year_month_day> DateOption(
    const CommandCall& call, std::string_view name,
    std::chrono::year_month_day fallback) {
  const std::optional<std::string_view> text = call.Option(name);
  if (!text.has_value()) {
    return fallback;
  }
  return core::ParseDate(*text);
}

core::Result<std::optional<std::chrono::year_month_day>> OptionalDateOption(
    const CommandCall& call, std::string_view name) {
  const std::optional<std::string_view> text = call.Option(name);
  if (!text.has_value()) {
    return std::nullopt;
  }
  return core::ParseDate(*text).transform([](std::chrono::year_month_day date) {
    return std::optional<std::chrono::year_month_day>(date);
  });
}

std::size_t DisplayWidth(std::string_view text) {
  // Count every byte except UTF-8 continuation bytes (binary 10xxxxxx).
  return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) {
    return (static_cast<unsigned char>(c) & 0xC0U) != 0x80U;
  }));
}

TextTable::TextTable(std::vector<Align> alignment)
    : alignment_(std::move(alignment)) {}

void TextTable::AddRow(std::vector<std::string> cells) {
  core::Check(cells.size() == alignment_.size(),
              "TextTable row has the wrong number of cells");
  rows_.push_back(std::move(cells));
}

std::string TextTable::Render() const {
  std::vector<std::size_t> widths(alignment_.size(), 0);
  for (const std::vector<std::string>& row : rows_) {
    for (std::size_t column = 0; column < row.size(); ++column) {
      widths.at(column) =
          std::max(widths.at(column), DisplayWidth(row.at(column)));
    }
  }

  std::string text;
  for (const std::vector<std::string>& row : rows_) {
    std::string line;
    for (std::size_t column = 0; column < row.size(); ++column) {
      const std::string& cell = row.at(column);
      const std::string padding(widths.at(column) - DisplayWidth(cell), ' ');
      if (column > 0) {
        line += "  ";
      }
      line += alignment_.at(column) == Align::kRight ? padding + cell
                                                     : cell + padding;
    }
    // No trailing spaces (left-aligned last columns are padded).
    line.erase(line.find_last_not_of(' ') + 1);
    text += line + '\n';
  }
  return text;
}

}  // namespace mymoney::cli::commands
