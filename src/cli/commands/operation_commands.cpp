// Commands that record operations: expense, income, transfer.

#include <chrono>
#include <expected>
#include <format>
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
#include "core/date_text.h"
#include "core/error.h"
#include "core/money.h"
#include "core/operation.h"

namespace mymoney::cli::commands {
namespace {

// A group cannot have operations (ADR-0015). OperationService refuses it too;
// this message also tells the user which categories to use instead.
core::Status CheckNotAGroup(Context& context, const core::Category& category) {
  if (!category.is_group) {
    return {};
  }
  return context.categories->List().and_then(
      [&](const std::vector<core::Category>& categories) -> core::Status {
        std::string names;
        for (const core::Category& child : categories) {
          if (child.parent_id == category.id) {
            names += names.empty() ? child.name : ", " + child.name;
          }
        }
        if (names.empty()) {
          return core::MakeError(
              core::ErrorCode::kRuleViolation,
              std::format("'{0}' is a group without categories; add one "
                          "with 'category add <name> --group \"{0}\"'",
                          category.name));
        }
        return core::MakeError(
            core::ErrorCode::kRuleViolation,
            std::format("'{}' is a group; use one of its categories: {}",
                        category.name, names));
      });
}

// expense and income: <account> <category> <amount> [--date] [--comment].
core::Result<std::string> AddSingleMovement(Context& context,
                                            const CommandCall& call,
                                            core::OperationType type) {
  const core::Result<core::Account> account =
      context.accounts->GetByName(call.arguments.at(0));
  if (!account) {
    return std::unexpected(account.error());
  }
  const core::Result<core::Category> category =
      context.categories->GetByName(call.arguments.at(1));
  if (!category) {
    return std::unexpected(category.error());
  }
  if (core::Status status = CheckNotAGroup(context, *category); !status) {
    return std::unexpected(std::move(status).error());
  }
  MoneyText money(context.currencies);
  const core::Result<core::Money> amount =
      money.Parse(call.arguments.at(2), account->currency);
  if (!amount) {
    return std::unexpected(amount.error());
  }
  const core::Result<std::chrono::year_month_day> date =
      DateOption(call, "date", context.today);
  if (!date) {
    return std::unexpected(date.error());
  }
  std::string comment(call.Option("comment").value_or(""));

  const bool expense = type == core::OperationType::kExpense;
  core::Result<core::OperationDraft> draft =
      expense ? core::OperationDraft::Expense(*date, account->id, category->id,
                                              *amount, std::move(comment))
              : core::OperationDraft::Income(*date, account->id, category->id,
                                             *amount, std::move(comment));
  return draft
      .and_then([&](const core::OperationDraft& valid) {
        return context.operation_service->Post(valid);
      })
      .and_then([&](const core::Operation& /*posted*/) {
        return money.Format(*amount);
      })
      .transform([&](const std::string& amount_text) {
        return std::format("Added {}: {} {} '{}' ({}) on {}.\n",
                           expense ? "expense" : "income", amount_text,
                           expense ? "from" : "to", account->name,
                           category->name, core::FormatDate(*date));
      });
}

}  // namespace

core::Result<std::string> Expense(Context& context, const CommandCall& call) {
  return AddSingleMovement(context, call, core::OperationType::kExpense);
}

core::Result<std::string> Income(Context& context, const CommandCall& call) {
  return AddSingleMovement(context, call, core::OperationType::kIncome);
}

// transfer <from> <to> <amount> [--amount-in AMOUNT] [--date] [--comment]
core::Result<std::string> Transfer(Context& context, const CommandCall& call) {
  const core::Result<core::Account> from =
      context.accounts->GetByName(call.arguments.at(0));
  if (!from) {
    return std::unexpected(from.error());
  }
  const core::Result<core::Account> to =
      context.accounts->GetByName(call.arguments.at(1));
  if (!to) {
    return std::unexpected(to.error());
  }
  MoneyText money(context.currencies);
  const core::Result<core::Money> amount_out =
      money.Parse(call.arguments.at(2), from->currency);
  if (!amount_out) {
    return std::unexpected(amount_out.error());
  }

  // Between currencies the received amount is independent (ADR-0006); in one
  // currency it is the same amount unless given explicitly.
  core::Result<core::Money> amount_in = *amount_out;
  if (const std::optional<std::string_view> text = call.Option("amount-in")) {
    amount_in = money.Parse(*text, to->currency);
  } else if (from->currency != to->currency) {
    return core::MakeError(
        core::ErrorCode::kInvalidArgument,
        std::format("'{}' is in {} and '{}' in {}: give the received amount "
                    "with --amount-in",
                    from->name, from->currency.code(), to->name,
                    to->currency.code()));
  }
  if (!amount_in) {
    return std::unexpected(amount_in.error());
  }
  const core::Result<std::chrono::year_month_day> date =
      DateOption(call, "date", context.today);
  if (!date) {
    return std::unexpected(date.error());
  }

  return core::OperationDraft::Transfer(
             *date, from->id, *amount_out, to->id, *amount_in,
             std::string(call.Option("comment").value_or("")))
      .and_then([&](const core::OperationDraft& valid) {
        return context.operation_service->Post(valid);
      })
      .and_then([&](const core::Operation& /*posted*/) {
        return money.Format(*amount_out);
      })
      .and_then([&](const std::string& out_text) -> core::Result<std::string> {
        if (amount_in->currency() == amount_out->currency()) {
          return std::format("Added transfer: {} from '{}' to '{}' on {}.\n",
                             out_text, from->name, to->name,
                             core::FormatDate(*date));
        }
        return money.Format(*amount_in)
            .transform([&](const std::string& in_text) {
              return std::format(
                  "Added transfer: {} from '{}' to '{}' ({}) on {}.\n",
                  out_text, from->name, to->name, in_text,
                  core::FormatDate(*date));
            });
      });
}

}  // namespace mymoney::cli::commands
