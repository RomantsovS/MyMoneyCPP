// Commands for the "catalog" data: currencies, accounts, categories, groups.

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
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/strong_id.h"

namespace mymoney::cli::commands {
namespace {

// "0".."4" -> the number; the range itself is checked by Currency::Create.
core::Result<int> ParseDigits(std::string_view text) {
  if (text.size() != 1 || text.front() < '0' || text.front() > '9') {
    return core::MakeError(
        core::ErrorCode::kInvalidArgument,
        std::format("--digits must be a number from 0 to 4, got '{}'", text));
  }
  return text.front() - '0';
}

core::Result<core::CategoryKind> ParseKind(std::string_view text) {
  if (text == "expense") {
    return core::CategoryKind::kExpense;
  }
  if (text == "income") {
    return core::CategoryKind::kIncome;
  }
  return core::MakeError(
      core::ErrorCode::kInvalidArgument,
      std::format("--kind must be 'expense' or 'income', got '{}'", text));
}

std::string_view KindText(core::CategoryKind kind) {
  return kind == core::CategoryKind::kExpense ? "expense" : "income";
}

}  // namespace

// --- Currencies
// ---------------------------------------------------------------

core::Result<std::string> CurrencyAdd(Context& context,
                                      const CommandCall& call) {
  return ParseDigits(call.Option("digits").value_or(""))
      .and_then([&](int digits) {
        return core::CurrencyCode::Parse(call.arguments.front())
            .and_then([&](core::CurrencyCode code) {
              return core::Currency::Create(code, digits);
            });
      })
      .and_then([&](const core::Currency& currency) {
        return context.currencies->Add(currency).transform([&] {
          return std::format("Added currency {} ({} decimal places).\n",
                             currency.code().code(), currency.minor_digits());
        });
      });
}

core::Result<std::string> CurrencyList(Context& context,
                                       const CommandCall& /*call*/) {
  return context.currencies->List().transform(
      [](const std::vector<core::Currency>& currencies) {
        if (currencies.empty()) {
          return std::string(
              "No currencies yet. Add one with 'currency add'.\n");
        }
        TextTable table({TextTable::Align::kLeft, TextTable::Align::kLeft});
        for (const core::Currency& currency : currencies) {
          table.AddRow(
              {std::string(currency.code().code()),
               std::format("{} decimal places", currency.minor_digits())});
        }
        return table.Render();
      });
}

// --- Accounts
// -----------------------------------------------------------------

core::Result<std::string> AccountAdd(Context& context,
                                     const CommandCall& call) {
  return core::CurrencyCode::Parse(call.Option("currency").value_or(""))
      .and_then([&](core::CurrencyCode currency) {
        return context.account_service->Create(
            {.name = std::string(call.arguments.front()),
             .currency = currency});
      })
      .transform([](const core::Account& account) {
        return std::format("Added account '{}' ({}).\n", account.name,
                           account.currency.code());
      });
}

core::Result<std::string> AccountList(Context& context,
                                      const CommandCall& /*call*/) {
  return context.accounts->List().transform(
      [](const std::vector<core::Account>& accounts) {
        if (accounts.empty()) {
          return std::string("No accounts yet. Add one with 'account add'.\n");
        }
        TextTable table({TextTable::Align::kLeft, TextTable::Align::kLeft,
                         TextTable::Align::kLeft});
        for (const core::Account& account : accounts) {
          table.AddRow({account.name, std::string(account.currency.code()),
                        account.archived ? "archived" : ""});
        }
        return table.Render();
      });
}

core::Result<std::string> AccountArchive(Context& context,
                                         const CommandCall& call) {
  return context.accounts->GetByName(call.arguments.front())
      .and_then([&](const core::Account& account) {
        return context.account_service->Archive(account.id).transform([&] {
          return std::format("Archived account '{}'.\n", account.name);
        });
      });
}

// --- Categories and groups (ADR-0015)
// ---------------------------------------------------------------

core::Result<std::string> GroupAdd(Context& context, const CommandCall& call) {
  return ParseKind(call.Option("kind").value_or(""))
      .and_then([&](core::CategoryKind kind) {
        return context.category_service->Create(
            {.name = std::string(call.arguments.front()),
             .kind = kind,
             .parent_id = std::nullopt,
             .is_group = true});
      })
      .transform([](const core::Category& group) {
        return std::format("Added {} group '{}'.\n", KindText(group.kind),
                           group.name);
      });
}

core::Result<std::string> CategoryAdd(Context& context,
                                      const CommandCall& call) {
  std::optional<core::Category> group;
  if (const std::optional<std::string_view> group_name = call.Option("group")) {
    core::Result<core::Category> found =
        context.categories->GetByName(*group_name);
    if (!found) {
      return std::unexpected(std::move(found).error());
    }
    group = std::move(*found);
  }
  // Inside a group, the kind is the group's; a different --kind is refused by
  // CategoryService.
  const std::optional<std::string_view> kind_text = call.Option("kind");
  if (!kind_text.has_value() && !group.has_value()) {
    return core::MakeError(core::ErrorCode::kInvalidArgument,
                           "--kind is required for a category outside a group");
  }
  const core::Result<core::CategoryKind> kind =
      kind_text.has_value() ? ParseKind(*kind_text)
                            : core::Result<core::CategoryKind>(group->kind);
  if (!kind) {
    return std::unexpected(kind.error());
  }

  return context.category_service
      ->Create(
          {.name = std::string(call.arguments.front()),
           .kind = *kind,
           .parent_id = group.transform(
               [](const core::Category& category) { return category.id; })})
      .transform([&](const core::Category& category) {
        return group.has_value()
                   ? std::format("Added {} category '{}' in group '{}'.\n",
                                 KindText(category.kind), category.name,
                                 group->name)
                   : std::format("Added {} category '{}'.\n",
                                 KindText(category.kind), category.name);
      });
}

core::Result<std::string> CategoryList(Context& context,
                                       const CommandCall& /*call*/) {
  return context.categories->List().transform(
      [](const std::vector<core::Category>& categories) {
        if (categories.empty()) {
          return std::string(
              "No categories yet. Add one with 'category add' or "
              "'group add'.\n");
        }
        // The tree: everything at the root, each group followed by its
        // categories, indented.
        const auto kind_column = [](const core::Category& category) {
          return category.is_group
                     ? std::format("{} group", KindText(category.kind))
                     : std::string(KindText(category.kind));
        };
        TextTable table({TextTable::Align::kLeft, TextTable::Align::kLeft});
        for (const core::Category& root : categories) {
          if (root.parent_id.has_value()) {
            continue;
          }
          table.AddRow({root.name, kind_column(root)});
          for (const core::Category& child : categories) {
            if (child.parent_id == root.id) {
              table.AddRow({"  " + child.name, kind_column(child)});
            }
          }
        }
        return table.Render();
      });
}

}  // namespace mymoney::cli::commands
