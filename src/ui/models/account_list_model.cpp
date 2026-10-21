#include "ui/models/account_list_model.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QJSEngine>
#include <QModelIndex>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "core/account.h"
#include "core/check.h"
#include "core/currency.h"
#include "core/error.h"
#include "core/money.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/services/balance_service.h"
#include "ui/money_format.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {

AccountListModel::AccountListModel(core::AccountRepository* accounts,
                                   core::CurrencyRepository* currencies,
                                   core::BalanceService* balances,
                                   QObject* parent)
    : QAbstractListModel(parent),
      accounts_(accounts),
      currencies_(currencies),
      balances_(balances) {
  core::Check(
      accounts_ != nullptr && currencies_ != nullptr && balances_ != nullptr,
      "AccountListModel needs its repositories and BalanceService");
}

AccountListModel* AccountListModel::create(QQmlEngine* qml_engine,
                                           QJSEngine* /*js_engine*/) {
  return SingletonInstance<AccountListModel>::ForEngine(qml_engine);
}

int AccountListModel::rowCount(const QModelIndex& parent) const {
  // A list has rows only under the invisible root.
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant AccountListModel::data(const QModelIndex& index, int role) const {
  if (!checkIndex(index, CheckIndexOption::IndexIsValid |
                             CheckIndexOption::ParentIsInvalid)) {
    return {};
  }
  const Row& row = rows_.at(static_cast<std::size_t>(index.row()));
  switch (role) {
    case kAccountIdRole:
      return QVariant::fromValue(static_cast<qint64>(row.id.value()));
    case Qt::DisplayRole:
    case kNameRole:
      return row.name;
    case kCurrencyRole:
      return QString::fromUtf8(row.balance.currency().code());
    case kBalanceTextRole:
      return FormatMoney(row.balance, row.minor_digits);
    case kArchivedRole:
      return row.archived;
    default:
      return {};
  }
}

QHash<int, QByteArray> AccountListModel::roleNames() const {
  return {{kAccountIdRole, "accountId"},
          {kNameRole, "name"},
          {kCurrencyRole, "currency"},
          {kBalanceTextRole, "balanceText"},
          {kArchivedRole, "archived"}};
}

void AccountListModel::setShowArchived(bool show) {
  if (show == show_archived_) {
    return;
  }
  show_archived_ = show;
  emit showArchivedChanged();
  ApplyFilter();
}

void AccountListModel::reload() {
  std::vector<Row> rows;
  // Read every account's currency only once.
  std::map<std::string, int, std::less<>> minor_digits;
  const core::Status status = accounts_->List().and_then(
      [&](const std::vector<core::Account>& accounts) -> core::Status {
        for (const core::Account& account : accounts) {
          const std::string code(account.currency.code());
          if (!minor_digits.contains(code)) {
            const core::Result<core::Currency> currency =
                currencies_->Get(account.currency);
            if (!currency) {
              return std::unexpected(currency.error());
            }
            minor_digits.emplace(code, currency->minor_digits());
          }
          const core::Result<core::Money> balance =
              balances_->GetBalance(account.id);
          if (!balance) {
            return std::unexpected(balance.error());
          }
          rows.push_back({.id = account.id,
                          .name = QString::fromStdString(account.name),
                          .balance = *balance,
                          .minor_digits = minor_digits.at(code),
                          .archived = account.archived});
        }
        return {};
      });

  if (status) {
    all_rows_ = std::move(rows);
    error_text_.clear();
  } else {
    all_rows_.clear();
    error_text_ = QString::fromStdString(status.error().message);
  }
  ApplyFilter();
}

void AccountListModel::ApplyFilter() {
  beginResetModel();
  rows_.clear();
  // Per currency: the sum and the number of decimal places, by code.
  std::map<std::string, std::pair<core::Money, int>, std::less<>> totals;
  for (const Row& row : all_rows_) {
    if (row.archived && !show_archived_) {
      continue;
    }
    rows_.push_back(row);
    const std::string code(row.balance.currency().code());
    if (const auto it = totals.find(code); it != totals.end()) {
      it->second.first = it->second.first + row.balance;
    } else {
      totals.emplace(code, std::pair{row.balance, row.minor_digits});
    }
  }
  endResetModel();

  totals_.clear();
  for (const auto& [code, total] : totals) {
    totals_.append(FormatMoney(total.first, total.second));
  }
  emit loaded();
}

}  // namespace mymoney::ui
