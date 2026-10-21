#include "ui/models/currency_list_model.h"

#include <cstddef>
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

#include "core/check.h"
#include "core/currency.h"
#include "core/error.h"
#include "core/repositories/currency_repository.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {

CurrencyListModel::CurrencyListModel(core::CurrencyRepository* currencies,
                                     QObject* parent)
    : QAbstractListModel(parent), currencies_(currencies) {
  core::Check(currencies_ != nullptr,
              "CurrencyListModel needs a CurrencyRepository");
}

CurrencyListModel* CurrencyListModel::create(QQmlEngine* qml_engine,
                                             QJSEngine* /*js_engine*/) {
  return SingletonInstance<CurrencyListModel>::ForEngine(qml_engine);
}

int CurrencyListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant CurrencyListModel::data(const QModelIndex& index, int role) const {
  if (!checkIndex(index, CheckIndexOption::IndexIsValid |
                             CheckIndexOption::ParentIsInvalid)) {
    return {};
  }
  const core::Currency& currency =
      rows_.at(static_cast<std::size_t>(index.row()));
  switch (role) {
    case Qt::DisplayRole:
    case kCodeRole:
      return QString::fromUtf8(currency.code().code());
    case kMinorDigitsRole:
      return currency.minor_digits();
    default:
      return {};
  }
}

QHash<int, QByteArray> CurrencyListModel::roleNames() const {
  return {{kCodeRole, "code"}, {kMinorDigitsRole, "minorDigits"}};
}

void CurrencyListModel::reload() {
  core::Result<std::vector<core::Currency>> currencies = currencies_->List();
  beginResetModel();
  if (currencies) {
    rows_ = std::move(*currencies);
    error_text_.clear();
  } else {
    rows_.clear();
    error_text_ = QString::fromStdString(currencies.error().message);
  }
  endResetModel();
  emit loaded();
}

}  // namespace mymoney::ui
