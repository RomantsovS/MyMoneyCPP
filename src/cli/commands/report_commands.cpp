// Read-only commands: journal, balance.

#include <chrono>
#include <expected>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cli/command_table.h"
#include "cli/commands/command_helpers.h"
#include "cli/commands/commands.h"
#include "cli/context.h"
#include "core/account.h"
#include "core/category.h"
#include "core/currency_code.h"
#include "core/date_text.h"
#include "core/error.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"

namespace mymoney::cli::commands {
namespace {

// Names for the ids an operation refers to.
struct Names {
  std::map<core::AccountId, std::string> accounts;
  std::map<core::CategoryId, std::string> categories;
};

core::Result<Names> LoadNames(Context& context) {
  Names names;
  return context.accounts->List()
      .and_then([&](const std::vector<core::Account>& accounts) {
        for (const core::Account& account : accounts) {
          names.accounts.emplace(account.id, account.name);
        }
        return context.categories->List();
      })
      .transform([&](const std::vector<core::Category>& categories) {
        for (const core::Category& category : categories) {
          names.categories.emplace(category.id, category.name);
        }
        return std::move(names);
      });
}

std::string_view TypeText(core::OperationType type) {
  switch (type) {
    case core::OperationType::kExpense:
      return "expense";
    case core::OperationType::kIncome:
      return "income";
    case core::OperationType::kTransfer:
      return "transfer";
  }
  return "?";
}

// One journal row: date, type, account(s), category, amount, comment.
core::Result<std::vector<std::string>> JournalRow(
    const core::Operation& operation, const Names& names, MoneyText& money) {
  const core::OperationDraft& draft = operation.draft;
  const core::Movement& first = draft.movements().front();
  const std::string date = core::FormatDate(draft.date());
  const std::string type(TypeText(draft.type()));

  if (draft.type() != core::OperationType::kTransfer) {
    return money.Format(first.amount).transform([&](std::string amount) {
      return std::vector<std::string>{
          date,
          type,
          names.accounts.at(first.account_id),
          first.category_id ? names.categories.at(*first.category_id) : "",
          std::move(amount),
          draft.comment()};
    });
  }

  // A transfer: "From → To" and "100.00 EUR" or "100.00 EUR → 108.50 USD".
  const core::Movement& second = draft.movements().back();
  const std::string accounts =
      std::format("{} → {}", names.accounts.at(first.account_id),
                  names.accounts.at(second.account_id));
  return money.Format(-first.amount)
      .and_then([&](const std::string& out) -> core::Result<std::string> {
        if (first.amount.currency() == second.amount.currency()) {
          return out;
        }
        return money.Format(second.amount)
            .transform([&](const std::string& in) {
              return std::format("{} → {}", out, in);
            });
      })
      .transform([&](std::string amount) {
        return std::vector<std::string>{
            date, type, accounts, "", std::move(amount), draft.comment()};
      });
}

}  // namespace

// journal [--from D] [--to D] [--account NAME]
core::Result<std::string> Journal(Context& context, const CommandCall& call) {
  core::OperationFilter filter;
  const auto from = OptionalDateOption(call, "from");
  const auto to = OptionalDateOption(call, "to");
  if (!from || !to) {
    return std::unexpected(!from ? from.error() : to.error());
  }
  filter.from = *from;
  filter.to = *to;
  if (const std::optional<std::string_view> name = call.Option("account")) {
    const core::Result<core::Account> account =
        context.accounts->GetByName(*name);
    if (!account) {
      return std::unexpected(account.error());
    }
    filter.account_id = account->id;
  }

  const core::Result<Names> names = LoadNames(context);
  if (!names) {
    return std::unexpected(names.error());
  }
  const core::Result<std::vector<core::Operation>> operations =
      context.operations->List(filter);
  if (!operations) {
    return std::unexpected(operations.error());
  }
  if (operations->empty()) {
    return std::string("No operations.\n");
  }

  using Align = TextTable::Align;
  TextTable table({Align::kLeft, Align::kLeft, Align::kLeft, Align::kLeft,
                   Align::kRight, Align::kLeft});
  MoneyText money(context.currencies);
  for (const core::Operation& operation : *operations) {
    core::Result<std::vector<std::string>> row =
        JournalRow(operation, *names, money);
    if (!row) {
      return std::unexpected(std::move(row).error());
    }
    table.AddRow(std::move(*row));
  }
  return table.Render();
}

namespace {

// The accounts `balance` shows: the named one (also if archived), or all
// active accounts.
core::Result<std::vector<core::Account>> AccountsToShow(
    Context& context, const CommandCall& call) {
  if (!call.arguments.empty()) {
    return context.accounts->GetByName(call.arguments.front())
        .transform([](core::Account account) {
          return std::vector<core::Account>{std::move(account)};
        });
  }
  return context.accounts->List().transform([](std::vector<core::Account> all) {
    std::erase_if(
        all, [](const core::Account& account) { return account.archived; });
    return all;
  });
}

// One "Total XXX" row per currency. Amounts in different currencies are
// never added together - conversion comes with exchange rates in M7.
core::Status AddTotals(const std::map<core::CurrencyCode, core::Money>& totals,
                       MoneyText& money, TextTable& table) {
  for (const auto& [currency, total] : totals) {
    const core::Result<std::string> text = money.Format(total);
    if (!text) {
      return std::unexpected(text.error());
    }
    table.AddRow({std::format("Total {}", currency.code()), *text});
  }
  return {};
}

}  // namespace

// balance [<account>] [--date D]
core::Result<std::string> Balance(Context& context, const CommandCall& call) {
  const auto on = OptionalDateOption(call, "date");
  if (!on) {
    return std::unexpected(on.error());
  }
  const core::Result<std::vector<core::Account>> accounts =
      AccountsToShow(context, call);
  if (!accounts) {
    return std::unexpected(accounts.error());
  }
  if (accounts->empty()) {
    return std::string("No accounts yet. Add one with 'account add'.\n");
  }

  TextTable table({TextTable::Align::kLeft, TextTable::Align::kRight});
  MoneyText money(context.currencies);
  std::map<core::CurrencyCode, core::Money> totals;
  for (const core::Account& account : *accounts) {
    const core::Result<core::Money> balance =
        context.balance_service->GetBalance(account.id, *on);
    const core::Result<std::string> text = balance.and_then(
        [&](const core::Money& value) { return money.Format(value); });
    if (!text) {
      return std::unexpected(text.error());
    }
    table.AddRow({account.name, *text});
    if (auto [total, inserted] =
            totals.try_emplace(balance->currency(), *balance);
        !inserted) {
      total->second += *balance;
    }
  }

  if (accounts->size() > 1) {
    if (core::Status status = AddTotals(totals, money, table); !status) {
      return std::unexpected(std::move(status).error());
    }
  }
  return table.Render();
}

}  // namespace mymoney::cli::commands
