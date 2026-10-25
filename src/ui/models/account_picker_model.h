#ifndef MYMONEY_UI_MODELS_ACCOUNT_PICKER_MODEL_H_
#define MYMONEY_UI_MODELS_ACCOUNT_PICKER_MODEL_H_

#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration>

#include "core/account.h"
#include "core/repositories/account_repository.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The accounts an operation may use: the active ones, in creation order
// (ADR-0017). Archived accounts get no new operations.
class AccountPickerModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QString errorText READ errorText NOTIFY loaded FINAL)

 public:
  // The roles QML reads, by name (see roleNames()). Qt passes roles as int.
  static constexpr int kAccountIdRole = Qt::UserRole + 1;
  static constexpr int kNameRole = Qt::UserRole + 2;
  static constexpr int kCurrencyRole = Qt::UserRole + 3;

  // The repository is not owned and must outlive the model. Call reload() to
  // fill it.
  explicit AccountPickerModel(core::AccountRepository* accounts,
                              QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static AccountPickerModel* create(QQmlEngine* qml_engine,
                                    QJSEngine* js_engine);

  // Without a default argument (Google style); call rowCount({}).
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  // Empty unless the last reload() failed.
  [[nodiscard]] QString errorText() const { return error_text_; }

  Q_INVOKABLE void reload();

 signals:
  void loaded();

 private:
  core::AccountRepository* accounts_;
  std::vector<core::Account> rows_;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_MODELS_ACCOUNT_PICKER_MODEL_H_
