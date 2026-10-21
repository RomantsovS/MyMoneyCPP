#ifndef MYMONEY_UI_MODELS_ACCOUNT_LIST_MODEL_H_
#define MYMONEY_UI_MODELS_ACCOUNT_LIST_MODEL_H_

#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtQmlIntegration>

#include "core/money.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/services/balance_service.h"
#include "core/strong_id.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The accounts with their current balances, for the Accounts page
// (ADR-0016). Archived accounts are hidden unless showArchived is on.
// totals holds one line per currency of the shown accounts - no conversion
// before exchange rates (M7).
class AccountListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(bool showArchived READ showArchived WRITE setShowArchived NOTIFY
                 showArchivedChanged FINAL)
  Q_PROPERTY(QStringList totals READ totals NOTIFY loaded FINAL)
  Q_PROPERTY(QString errorText READ errorText NOTIFY loaded FINAL)

 public:
  // The roles QML reads, by name (see roleNames()). Qt passes roles as int.
  static constexpr int kAccountIdRole = Qt::UserRole + 1;
  static constexpr int kNameRole = Qt::UserRole + 2;
  static constexpr int kCurrencyRole = Qt::UserRole + 3;
  static constexpr int kBalanceTextRole = Qt::UserRole + 4;
  static constexpr int kArchivedRole = Qt::UserRole + 5;

  // The repositories and the service are not owned and must outlive the
  // model. Call reload() to fill it.
  AccountListModel(core::AccountRepository* accounts,
                   core::CurrencyRepository* currencies,
                   core::BalanceService* balances, QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static AccountListModel* create(QQmlEngine* qml_engine, QJSEngine* js_engine);

  // Without a default argument (Google style); call rowCount({}).
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] bool showArchived() const { return show_archived_; }
  void setShowArchived(bool show);
  [[nodiscard]] QStringList totals() const { return totals_; }
  // Empty unless the last reload() failed.
  [[nodiscard]] QString errorText() const { return error_text_; }

  // Reads all accounts and their balances again.
  Q_INVOKABLE void reload();

 signals:
  void showArchivedChanged();
  // After every reload() or filter change: totals and errorText may differ.
  void loaded();

 private:
  struct Row {
    core::AccountId id;
    QString name;
    core::Money balance;
    int minor_digits;
    bool archived;
  };

  // Shows the rows of all_rows_ that pass the filter and computes totals_.
  void ApplyFilter();

  core::AccountRepository* accounts_;
  core::CurrencyRepository* currencies_;
  core::BalanceService* balances_;

  bool show_archived_ = false;
  std::vector<Row> all_rows_;
  std::vector<Row> rows_;  // the shown rows
  QStringList totals_;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_MODELS_ACCOUNT_LIST_MODEL_H_
