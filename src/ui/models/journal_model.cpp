#include "ui/models/journal_model.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <expected>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QJSEngine>
#include <QLocale>
#include <QModelIndex>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtGlobal>

#include "core/account.h"
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
#include "core/strong_id.h"
#include "ui/date_convert.h"
#include "ui/money_format.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {
namespace {

// What a row needs to know about an account.
struct AccountInfo {
  QString name;
  int minor_digits;
};

using AccountNames = std::map<core::AccountId, AccountInfo>;
using CategoryNames = std::map<core::CategoryId, QString>;

QVariantMap Choice(qint64 id, const QString& name) {
  return {{QStringLiteral("id"), QVariant::fromValue(id)},
          {QStringLiteral("name"), name}};
}

QString KindText(core::OperationType type) {
  switch (type) {
    case core::OperationType::kExpense:
      return QStringLiteral("expense");
    case core::OperationType::kIncome:
      return QStringLiteral("income");
    case core::OperationType::kTransfer:
      return QStringLiteral("transfer");
  }
  return {};
}

// "Октябрь 2026": the standalone month name, capitalized (Russian names are
// lower case).
QString MonthTitle(std::chrono::year_month month) {
  QString name = QLocale().standaloneMonthName(
      static_cast<int>(static_cast<unsigned>(month.month())));
  if (!name.isEmpty()) {
    name.front() = name.front().toUpper();
  }
  return QStringLiteral("%1 %2").arg(name).arg(static_cast<int>(month.year()));
}

// Every account with its name and its currency's decimal places.
core::Result<AccountNames> LoadAccounts(core::AccountRepository& accounts,
                                        core::CurrencyRepository& currencies) {
  return accounts.List().and_then(
      [&](const std::vector<core::Account>& all) -> core::Result<AccountNames> {
        AccountNames names;
        for (const core::Account& account : all) {
          const core::Result<core::Currency> currency =
              currencies.Get(account.currency);
          if (!currency) {
            return std::unexpected(currency.error());
          }
          names.emplace(
              account.id,
              AccountInfo{.name = QString::fromStdString(account.name),
                          .minor_digits = currency->minor_digits()});
        }
        return names;
      });
}

// "Any account", then every account - also archived ones - in id order.
QVariantList AccountChoices(const AccountNames& accounts) {
  QVariantList choices{Choice(-1, JournalModel::tr("Any account"))};
  for (const auto& [id, account] : accounts) {
    choices.append(Choice(id.value(), account.name));
  }
  return choices;
}

CategoryNames NamesOf(const std::vector<core::Category>& categories) {
  CategoryNames names;
  for (const core::Category& category : categories) {
    names.emplace(category.id, QString::fromStdString(category.name));
  }
  return names;
}

// Like the Categories page, for one kind: root categories, then each group
// followed by its categories (indented).
void AppendCategoryChoices(const std::vector<core::Category>& all,
                           core::CategoryKind kind, QVariantList* choices) {
  const auto choice = [](const core::Category& category, bool indented) {
    const QString name = QString::fromStdString(category.name);
    return Choice(category.id.value(),
                  indented ? QStringLiteral("    ") + name : name);
  };
  for (const core::Category& category : all) {
    if (category.kind == kind && !category.is_group &&
        !category.parent_id.has_value()) {
      choices->append(choice(category, false));
    }
  }
  for (const core::Category& group : all) {
    if (group.kind != kind || !group.is_group) {
      continue;
    }
    choices->append(choice(group, false));
    for (const core::Category& category : all) {
      if (category.parent_id == group.id) {
        choices->append(choice(category, true));
      }
    }
  }
}

// "Any category", then the expense and the income tree.
QVariantList CategoryChoices(const std::vector<core::Category>& all) {
  QVariantList choices{Choice(-1, JournalModel::tr("Any category"))};
  AppendCategoryChoices(all, core::CategoryKind::kExpense, &choices);
  AppendCategoryChoices(all, core::CategoryKind::kIncome, &choices);
  return choices;
}

// The texts of a journal row that depend on the operation's type.
struct RowTexts {
  QString title;    // the category, or "From → To"
  QString account;  // empty for a transfer (it is in the title)
  QString amount;   // with its sign; "a → b" between currencies
};

RowTexts TextsOf(const core::OperationDraft& draft,
                 const AccountNames& accounts,
                 const CategoryNames& categories) {
  const core::Movement& first = draft.movements().front();
  const AccountInfo& from = accounts.at(first.account_id);
  if (draft.type() == core::OperationType::kTransfer) {
    const core::Movement& in = draft.movements().back();
    const AccountInfo& to = accounts.at(in.account_id);
    QString amount = FormatMoney(-first.amount, from.minor_digits);
    if (in.amount.currency() != first.amount.currency()) {
      amount += QStringLiteral(" → ") + FormatMoney(in.amount, to.minor_digits);
    }
    return {.title = QStringLiteral("%1 → %2").arg(from.name, to.name),
            .account = {},
            .amount = amount};
  }
  // An expense is stored negative, so it shows its minus sign.
  QString amount = FormatMoney(first.amount, from.minor_digits);
  if (first.amount.IsPositive()) {
    amount.prepend(QLocale().positiveSign());
  }
  return {.title = first.category_id.has_value()
                       ? categories.at(*first.category_id)
                       : QString(),
          .account = from.name,
          .amount = amount};
}

// The month of today's date - the first month the journal shows.
std::chrono::year_month CurrentMonth(const JournalModel::TodayFunction& today) {
  core::Check(today != nullptr, "JournalModel needs today");
  const std::chrono::year_month_day date = today();
  return date.year() / date.month();
}

}  // namespace

JournalModel::JournalModel(core::AccountRepository* accounts,
                           core::CategoryRepository* categories,
                           core::CurrencyRepository* currencies,
                           core::OperationRepository* operations,
                           TodayFunction today, QObject* parent)
    : QAbstractListModel(parent),
      accounts_(accounts),
      categories_(categories),
      currencies_(currencies),
      operations_(operations),
      today_(std::move(today)),
      month_(CurrentMonth(today_)) {
  core::Check(accounts_ != nullptr && categories_ != nullptr &&
                  currencies_ != nullptr && operations_ != nullptr,
              "JournalModel needs its repositories");
}

JournalModel* JournalModel::create(QQmlEngine* qml_engine,
                                   QJSEngine* /*js_engine*/) {
  return SingletonInstance<JournalModel>::ForEngine(qml_engine);
}

int JournalModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant JournalModel::data(const QModelIndex& index, int role) const {
  if (!checkIndex(index, CheckIndexOption::IndexIsValid |
                             CheckIndexOption::ParentIsInvalid)) {
    return {};
  }
  const Row& row = rows_.at(static_cast<std::size_t>(index.row()));
  switch (role) {
    case kOperationIdRole:
      return QVariant::fromValue(static_cast<qint64>(row.id.value()));
    case kDayTextRole:
      return row.day_text;
    case Qt::DisplayRole:
    case kTitleRole:
      return row.title;
    case kAccountRole:
      return row.account;
    case kAmountTextRole:
      return row.amount_text;
    case kKindRole:
      return row.kind;
    case kCommentRole:
      return row.comment;
    default:
      return {};
  }
}

QHash<int, QByteArray> JournalModel::roleNames() const {
  return {{kOperationIdRole, "operationId"},
          {kDayTextRole, "dayText"},
          {kTitleRole, "title"},
          {kAccountRole, "account"},
          {kAmountTextRole, "amountText"},
          {kKindRole, "kind"},
          {kCommentRole, "comment"}};
}

int JournalModel::year() const { return static_cast<int>(month_.year()); }

int JournalModel::month() const {
  return static_cast<int>(static_cast<unsigned>(month_.month()));
}

QString JournalModel::monthTitle() const { return MonthTitle(month_); }

void JournalModel::setAccountId(qint64 account_id) {
  if (account_id != account_id_) {
    account_id_ = account_id;
    reload();
  }
}

QString JournalModel::accountName() const {
  for (const QVariant& choice : account_choices_) {
    const QVariantMap map = choice.toMap();
    if (map.value(QStringLiteral("id")).toLongLong() == account_id_) {
      return map.value(QStringLiteral("name")).toString();
    }
  }
  return {};
}

void JournalModel::setCategoryId(qint64 category_id) {
  if (category_id != category_id_) {
    category_id_ = category_id;
    reload();
  }
}

QString JournalModel::categoryName() const {
  for (const QVariant& choice : category_choices_) {
    const QVariantMap map = choice.toMap();
    if (map.value(QStringLiteral("id")).toLongLong() == category_id_) {
      return map.value(QStringLiteral("name")).toString().trimmed();
    }
  }
  return {};
}

void JournalModel::setCommentText(const QString& text) {
  if (text != comment_text_) {
    comment_text_ = text;
    reload();
  }
}

void JournalModel::previousMonth() {
  month_ -= std::chrono::months{1};
  reload();
}

void JournalModel::nextMonth() {
  month_ += std::chrono::months{1};
  reload();
}

void JournalModel::reload() {
  std::vector<Row> rows;
  QVariantList account_choices;
  QVariantList category_choices;
  bool truncated = false;

  // The names behind the ids, then the operations.
  const core::Status status =
      LoadAccounts(*accounts_, *currencies_)
          .and_then([&](const AccountNames& accounts) {
            account_choices = AccountChoices(accounts);
            return categories_->List().and_then(
                [&](const std::vector<core::Category>& all) {
                  category_choices = CategoryChoices(all);
                  return operations_->List(Filter()).transform(
                      [&](const std::vector<core::Operation>& operations) {
                        // Only a search is limited; a month shows all.
                        const std::size_t shown =
                            searching() ? std::min<std::size_t>(
                                              operations.size(), kSearchLimit)
                                        : operations.size();
                        truncated = shown < operations.size();
                        const CategoryNames categories = NamesOf(all);
                        for (std::size_t i = 0; i < shown; ++i) {
                          const core::Operation& operation = operations.at(i);
                          const core::OperationDraft& draft = operation.draft;
                          RowTexts texts = TextsOf(draft, accounts, categories);
                          rows.push_back(
                              {.id = operation.id,
                               .day_text = DayText(draft.date()),
                               .title = std::move(texts.title),
                               .account = std::move(texts.account),
                               .amount_text = std::move(texts.amount),
                               .kind = KindText(draft.type()),
                               .comment =
                                   QString::fromStdString(draft.comment())});
                        }
                      });
                });
          });

  beginResetModel();
  if (status) {
    rows_ = std::move(rows);
    error_text_.clear();
  } else {
    rows_.clear();
    error_text_ = QString::fromStdString(status.error().message);
  }
  endResetModel();
  truncated_ = truncated;
  account_choices_ = std::move(account_choices);
  category_choices_ = std::move(category_choices);
  emit loaded();
}

core::OperationFilter JournalModel::Filter() const {
  core::OperationFilter filter{.order = core::OperationOrder::kNewestFirst};
  if (account_id_ >= 0) {
    filter.account_id = core::AccountId(account_id_);
  }
  if (category_id_ >= 0) {
    filter.category_id = core::CategoryId(category_id_);
  }
  if (searching()) {
    // The whole history; one more than shown tells whether it was cut.
    filter.comment_contains = comment_text_.trimmed().toStdString();
    filter.limit = kSearchLimit + 1;
  } else {
    filter.from = month_ / std::chrono::day{1};
    filter.to = std::chrono::year_month_day{month_ / std::chrono::last};
  }
  return filter;
}

QString JournalModel::DayText(std::chrono::year_month_day date) const {
  const std::chrono::year_month_day today = today_();
  if (date == today) {
    return tr("Today");
  }
  if (date == std::chrono::year_month_day{std::chrono::sys_days{today} -
                                          std::chrono::days{1}}) {
    return tr("Yesterday");
  }
  return QLocale().toString(ToQDate(date),
                            date.year() == today.year()
                                ? QStringLiteral("ddd, d MMM")
                                : QStringLiteral("ddd, d MMM yyyy"));
}

}  // namespace mymoney::ui
