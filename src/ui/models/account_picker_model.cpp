#include "ui/models/account_picker_model.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
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
#include <QVariant>

#include "core/account.h"
#include "core/check.h"
#include "core/error.h"
#include "core/repositories/account_repository.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {

AccountPickerModel::AccountPickerModel(core::AccountRepository* accounts,
                                       QObject* parent)
    : QAbstractListModel(parent), accounts_(accounts) {
  core::Check(accounts_ != nullptr,
              "AccountPickerModel needs an AccountRepository");
}

AccountPickerModel* AccountPickerModel::create(QQmlEngine* qml_engine,
                                               QJSEngine* /*js_engine*/) {
  return SingletonInstance<AccountPickerModel>::ForEngine(qml_engine);
}

int AccountPickerModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant AccountPickerModel::data(const QModelIndex& index, int role) const {
  if (!checkIndex(index, CheckIndexOption::IndexIsValid |
                             CheckIndexOption::ParentIsInvalid)) {
    return {};
  }
  const core::Account& account =
      rows_.at(static_cast<std::size_t>(index.row()));
  switch (role) {
    case kAccountIdRole:
      return QVariant::fromValue(static_cast<qint64>(account.id.value()));
    case Qt::DisplayRole:
    case kNameRole:
      return QString::fromStdString(account.name);
    case kCurrencyRole:
      return QString::fromUtf8(account.currency.code());
    default:
      return {};
  }
}

QHash<int, QByteArray> AccountPickerModel::roleNames() const {
  return {{kAccountIdRole, "accountId"},
          {kNameRole, "name"},
          {kCurrencyRole, "currency"}};
}

void AccountPickerModel::reload() {
  core::Result<std::vector<core::Account>> accounts = accounts_->List();
  beginResetModel();
  rows_.clear();
  if (accounts) {
    std::ranges::copy_if(
        *accounts, std::back_inserter(rows_),
        [](const core::Account& account) { return !account.archived; });
    error_text_.clear();
  } else {
    error_text_ = QString::fromStdString(std::move(accounts).error().message);
  }
  endResetModel();
  emit loaded();
}

}  // namespace mymoney::ui
