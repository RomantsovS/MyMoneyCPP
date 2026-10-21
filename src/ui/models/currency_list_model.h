#ifndef MYMONEY_UI_MODELS_CURRENCY_LIST_MODEL_H_
#define MYMONEY_UI_MODELS_CURRENCY_LIST_MODEL_H_

#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration>

#include "core/currency.h"
#include "core/repositories/currency_repository.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The known currencies, by code - for the currency picker (ADR-0016).
class CurrencyListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QString errorText READ errorText NOTIFY loaded FINAL)

 public:
  // The roles QML reads, by name (see roleNames()). Qt passes roles as int.
  static constexpr int kCodeRole = Qt::UserRole + 1;
  static constexpr int kMinorDigitsRole = Qt::UserRole + 2;

  // The repository is not owned and must outlive the model. Call reload() to
  // fill it.
  explicit CurrencyListModel(core::CurrencyRepository* currencies,
                             QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static CurrencyListModel* create(QQmlEngine* qml_engine,
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
  core::CurrencyRepository* currencies_;
  std::vector<core::Currency> rows_;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_MODELS_CURRENCY_LIST_MODEL_H_
