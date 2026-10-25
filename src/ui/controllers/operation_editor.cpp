#include "ui/controllers/operation_editor.h"

#include <chrono>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QJSEngine>
#include <QLocale>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtGlobal>

#include "core/account.h"
#include "core/amount_text.h"
#include "core/category.h"
#include "core/check.h"
#include "core/currency.h"
#include "core/error.h"
#include "core/money.h"
#include "core/operation.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/services/operation_service.h"
#include "core/strong_id.h"
#include "ui/date_convert.h"
#include "ui/money_format.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {
namespace {

std::optional<core::OperationType> TypeFromText(const QString& text) {
  if (text == u"expense") {
    return core::OperationType::kExpense;
  }
  if (text == u"income") {
    return core::OperationType::kIncome;
  }
  if (text == u"transfer") {
    return core::OperationType::kTransfer;
  }
  return std::nullopt;
}

QString TypeTitle(core::OperationType type) {
  switch (type) {
    case core::OperationType::kExpense:
      return OperationEditor::tr("Expense");
    case core::OperationType::kIncome:
      return OperationEditor::tr("Income");
    case core::OperationType::kTransfer:
      return OperationEditor::tr("Transfer");
  }
  return {};
}

core::CategoryKind KindOf(core::OperationType type) {
  return type == core::OperationType::kIncome ? core::CategoryKind::kIncome
                                              : core::CategoryKind::kExpense;
}

std::unexpected<core::Error> Incomplete(std::string_view what) {
  return core::MakeError(core::ErrorCode::kInvalidArgument, std::string(what));
}

}  // namespace

std::optional<std::int64_t> ParsePositiveAmount(const QString& text,
                                                int minor_digits) {
  const core::Result<std::int64_t> minor_units =
      core::ParseAmount(text.trimmed().toStdString(), minor_digits);
  if (!minor_units || *minor_units <= 0) {
    return std::nullopt;
  }
  return *minor_units;
}

OperationEditor::OperationEditor(core::AccountRepository* accounts,
                                 core::CategoryRepository* categories,
                                 core::CurrencyRepository* currencies,
                                 core::OperationRepository* operations,
                                 core::OperationService* service,
                                 TodayFunction today, QObject* parent)
    : QObject(parent),
      accounts_(accounts),
      categories_(categories),
      currencies_(currencies),
      operations_(operations),
      service_(service),
      today_(std::move(today)) {
  core::Check(accounts_ != nullptr && categories_ != nullptr &&
                  currencies_ != nullptr && operations_ != nullptr &&
                  service_ != nullptr && today_ != nullptr,
              "OperationEditor needs its repositories, service and today");
  date_ = today_();
}

OperationEditor* OperationEditor::create(QQmlEngine* qml_engine,
                                         QJSEngine* /*js_engine*/) {
  return SingletonInstance<OperationEditor>::ForEngine(qml_engine);
}

// --- Properties
// ---------------------------------------------------------------

QString OperationEditor::type() const {
  switch (type_) {
    case core::OperationType::kExpense:
      return QStringLiteral("expense");
    case core::OperationType::kIncome:
      return QStringLiteral("income");
    case core::OperationType::kTransfer:
      return QStringLiteral("transfer");
  }
  return {};
}

void OperationEditor::setType(const QString& type) {
  const std::optional<core::OperationType> new_type = TypeFromText(type);
  if (!new_type.has_value() || *new_type == type_) {
    return;
  }
  type_ = *new_type;
  category_.reset();  // a category belongs to one kind
  if (accounts_chosen_) {
    // The user's own choice stays; a transfer still needs a second account.
    if (type_ == core::OperationType::kTransfer) {
      FillToAccount();
    }
  } else {
    ApplyDefaultAccounts();
  }
  LoadRecentCategories();
  emit changed();
}

void OperationEditor::setAmountText(const QString& text) {
  if (text != amount_text_) {
    amount_text_ = text;
    emit changed();
  }
}

void OperationEditor::setAmountInText(const QString& text) {
  if (text != amount_in_text_) {
    amount_in_text_ = text;
    emit changed();
  }
}

bool OperationEditor::needsAmountIn() const {
  return type_ == core::OperationType::kTransfer && account_.has_value() &&
         to_account_.has_value() &&
         account_->account.currency != to_account_->account.currency;
}

qint64 OperationEditor::accountId() const {
  return account_.has_value() ? account_->account.id.value() : -1;
}

QString OperationEditor::accountName() const {
  return account_.has_value() ? QString::fromStdString(account_->account.name)
                              : QString();
}

QString OperationEditor::currencyCode() const {
  return account_.has_value()
             ? QString::fromUtf8(account_->account.currency.code())
             : QString();
}

qint64 OperationEditor::toAccountId() const {
  return to_account_.has_value() ? to_account_->account.id.value() : -1;
}

QString OperationEditor::toAccountName() const {
  return to_account_.has_value()
             ? QString::fromStdString(to_account_->account.name)
             : QString();
}

QString OperationEditor::toCurrencyCode() const {
  return to_account_.has_value()
             ? QString::fromUtf8(to_account_->account.currency.code())
             : QString();
}

qint64 OperationEditor::categoryId() const {
  return category_.has_value() ? category_->id.value() : -1;
}

QString OperationEditor::categoryName() const {
  return category_.has_value() ? QString::fromStdString(category_->name)
                               : QString();
}

QString OperationEditor::dateText() const {
  return QLocale().toString(ToQDate(date_), QLocale::ShortFormat);
}

bool OperationEditor::isYesterday() const {
  return date_ == std::chrono::year_month_day{std::chrono::sys_days{today_()} -
                                              std::chrono::days{1}};
}

int OperationEditor::year() const { return static_cast<int>(date_.year()); }

int OperationEditor::month() const {
  return static_cast<int>(static_cast<unsigned>(date_.month()));
}

int OperationEditor::day() const {
  return static_cast<int>(static_cast<unsigned>(date_.day()));
}

void OperationEditor::setComment(const QString& comment) {
  if (comment != comment_) {
    comment_ = comment;
    emit changed();
  }
}

bool OperationEditor::valid() const {
  if (!account_.has_value() ||
      !ParsePositiveAmount(amount_text_, account_->minor_digits)) {
    return false;
  }
  if (type_ != core::OperationType::kTransfer) {
    return category_.has_value();
  }
  return to_account_.has_value() &&
         to_account_->account.id != account_->account.id &&
         (!needsAmountIn() ||
          ParsePositiveAmount(amount_in_text_, to_account_->minor_digits));
}

// --- Actions
// ------------------------------------------------------------------

void OperationEditor::startNew() {
  type_ = core::OperationType::kExpense;
  amount_text_.clear();
  amount_in_text_.clear();
  category_.reset();
  date_ = today_();
  comment_.clear();
  error_text_.clear();
  saved_text_.clear();
  accounts_chosen_ = false;
  ApplyDefaultAccounts();
  LoadRecentCategories();
  emit changed();
}

void OperationEditor::chooseAccount(qint64 account_id) {
  std::optional<ChosenAccount> account =
      LoadAccount(core::AccountId(account_id));
  if (!account.has_value() || account->account.archived) {
    return;
  }
  account_ = std::move(account);
  accounts_chosen_ = true;
  emit changed();
}

void OperationEditor::chooseToAccount(qint64 account_id) {
  std::optional<ChosenAccount> account =
      LoadAccount(core::AccountId(account_id));
  if (!account.has_value() || account->account.archived) {
    return;
  }
  to_account_ = std::move(account);
  accounts_chosen_ = true;
  emit changed();
}

void OperationEditor::chooseCategory(qint64 category_id) {
  core::Result<core::Category> category =
      categories_->Get(core::CategoryId(category_id));
  // Only a category of this operation's kind - never a group (ADR-0015).
  if (!category || category->is_group || category->kind != KindOf(type_)) {
    return;
  }
  category_ = std::move(*category);
  emit changed();
}

void OperationEditor::setToday() {
  date_ = today_();
  emit changed();
}

void OperationEditor::setYesterday() {
  date_ = std::chrono::year_month_day{std::chrono::sys_days{today_()} -
                                      std::chrono::days{1}};
  emit changed();
}

void OperationEditor::setDate(int year, int month, int day) {
  const std::chrono::year_month_day date{
      std::chrono::year{year}, std::chrono::month{static_cast<unsigned>(month)},
      std::chrono::day{static_cast<unsigned>(day)}};
  if (date.ok()) {
    date_ = date;
    emit changed();
  }
}

bool OperationEditor::save() {
  core::Result<core::Operation> posted = BuildDraft().and_then(
      [&](const core::OperationDraft& draft) { return service_->Post(draft); });
  if (!posted || !account_.has_value()) {
    error_text_ =
        posted ? QString() : QString::fromStdString(posted.error().message);
    saved_text_.clear();
    emit changed();
    return false;
  }
  // The first movement is on the chosen account: negative for an expense and
  // for the source of a transfer.
  const core::Money& moved = posted->draft.movements().front().amount;
  saved_text_ = tr("%1 %2 saved")
                    .arg(TypeTitle(type_),
                         FormatMoney(moved.IsNegative() ? -moved : moved,
                                     account_->minor_digits));
  error_text_.clear();
  emit changed();
  emit saved();
  return true;
}

// --- Helpers
// ------------------------------------------------------------------

std::optional<OperationEditor::ChosenAccount> OperationEditor::LoadAccount(
    core::AccountId id) const {
  core::Result<core::Account> account = accounts_->Get(id);
  if (!account) {
    return std::nullopt;
  }
  const core::Result<core::Currency> currency =
      currencies_->Get(account->currency);
  if (!currency) {
    return std::nullopt;
  }
  return ChosenAccount{.account = std::move(*account),
                       .minor_digits = currency->minor_digits()};
}

std::optional<OperationEditor::ChosenAccount>
OperationEditor::FirstActiveAccount(
    std::optional<core::AccountId> except) const {
  const core::Result<std::vector<core::Account>> accounts = accounts_->List();
  if (!accounts) {
    return std::nullopt;
  }
  for (const core::Account& account : *accounts) {
    if (!account.archived && account.id != except) {
      return LoadAccount(account.id);
    }
  }
  return std::nullopt;
}

void OperationEditor::ApplyDefaultAccounts() {
  const auto active = [](std::optional<ChosenAccount> account) {
    return account.has_value() && !account->account.archived
               ? std::move(account)
               : std::nullopt;
  };
  account_.reset();
  to_account_.reset();
  if (const core::Result<core::Operation> latest = operations_->Latest(type_)) {
    const std::vector<core::Movement>& movements = latest->draft.movements();
    // Expense and income: one movement; transfer: first out, then in.
    account_ = active(LoadAccount(movements.front().account_id));
    if (type_ == core::OperationType::kTransfer) {
      to_account_ = active(LoadAccount(movements.back().account_id));
    }
  }
  if (!account_.has_value()) {
    account_ = FirstActiveAccount(std::nullopt);
  }
  if (type_ == core::OperationType::kTransfer) {
    FillToAccount();
  }
}

void OperationEditor::FillToAccount() {
  const std::optional<core::AccountId> from =
      account_.has_value() ? std::optional(account_->account.id) : std::nullopt;
  if (!to_account_.has_value() || to_account_->account.id == from) {
    to_account_ = FirstActiveAccount(from);
  }
}

void OperationEditor::LoadRecentCategories() {
  recent_categories_.clear();
  const core::Result<std::vector<core::CategoryId>> ids =
      operations_->RecentCategories(type_, kRecentCategoryCount);
  if (!ids) {
    return;  // only a convenience: the picker still offers every category
  }
  for (const core::CategoryId id : *ids) {
    if (const core::Result<core::Category> category = categories_->Get(id)) {
      recent_categories_.append(QVariantMap{
          {QStringLiteral("categoryId"),
           QVariant::fromValue(static_cast<qint64>(id.value()))},
          {QStringLiteral("name"), QString::fromStdString(category->name)}});
    }
  }
}

core::Result<core::OperationDraft> OperationEditor::BuildDraft() const {
  if (!account_.has_value()) {
    return Incomplete("choose an account");
  }
  const std::optional<std::int64_t> minor_units =
      ParsePositiveAmount(amount_text_, account_->minor_digits);
  if (!minor_units.has_value()) {
    return Incomplete("enter a positive amount");
  }
  const core::Money amount(*minor_units, account_->account.currency);
  std::string comment = comment_.trimmed().toStdString();

  if (type_ != core::OperationType::kTransfer) {
    if (!category_.has_value()) {
      return Incomplete("choose a category");
    }
    return type_ == core::OperationType::kExpense
               ? core::OperationDraft::Expense(date_, account_->account.id,
                                               category_->id, amount,
                                               std::move(comment))
               : core::OperationDraft::Income(date_, account_->account.id,
                                              category_->id, amount,
                                              std::move(comment));
  }

  if (!to_account_.has_value()) {
    return Incomplete("choose the target account");
  }
  // Same currency: the target receives the same amount (ADR-0006).
  const std::optional<std::int64_t> minor_units_in =
      needsAmountIn()
          ? ParsePositiveAmount(amount_in_text_, to_account_->minor_digits)
          : minor_units;
  if (!minor_units_in.has_value()) {
    return Incomplete("enter the amount received");
  }
  return core::OperationDraft::Transfer(
      date_, account_->account.id, amount, to_account_->account.id,
      core::Money(*minor_units_in, to_account_->account.currency),
      std::move(comment));
}

}  // namespace mymoney::ui
