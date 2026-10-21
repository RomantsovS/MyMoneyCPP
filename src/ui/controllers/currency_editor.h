#ifndef MYMONEY_UI_CONTROLLERS_CURRENCY_EDITOR_H_
#define MYMONEY_UI_CONTROLLERS_CURRENCY_EDITOR_H_

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

#include "core/repositories/currency_repository.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The state behind the currency form (ADR-0016): an ISO 4217 code and the
// number of decimal places. A fresh phone has no currencies, so this form is
// reachable from the account form.
class CurrencyEditor : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  // Stored in upper case, so "eur" typed on a phone becomes "EUR".
  Q_PROPERTY(QString code READ code WRITE setCode NOTIFY changed FINAL)
  Q_PROPERTY(int minorDigits READ minorDigits WRITE setMinorDigits NOTIFY
                 changed FINAL)
  // Three letters; the core checks the rest on save().
  Q_PROPERTY(bool valid READ valid NOTIFY changed FINAL)
  // Empty unless the last save() failed.
  Q_PROPERTY(QString errorText READ errorText NOTIFY changed FINAL)

 public:
  static constexpr int kDefaultMinorDigits = 2;

  // The repository is not owned and must outlive the editor.
  explicit CurrencyEditor(core::CurrencyRepository* currencies,
                          QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static CurrencyEditor* create(QQmlEngine* qml_engine, QJSEngine* js_engine);

  [[nodiscard]] QString code() const { return code_; }
  void setCode(const QString& code);
  [[nodiscard]] int minorDigits() const { return minor_digits_; }
  void setMinorDigits(int digits);
  [[nodiscard]] bool valid() const { return code_.size() == 3; }
  [[nodiscard]] QString errorText() const { return error_text_; }

  // An empty form: no code, kDefaultMinorDigits.
  Q_INVOKABLE void startNew();
  // Adds the currency. True and saved(code) on success; false and errorText
  // otherwise.
  Q_INVOKABLE bool save();

 signals:
  void changed();
  void saved(const QString& code);

 private:
  core::CurrencyRepository* currencies_;

  QString code_;
  int minor_digits_ = kDefaultMinorDigits;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_CONTROLLERS_CURRENCY_EDITOR_H_
