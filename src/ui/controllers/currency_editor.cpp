#include "ui/controllers/currency_editor.h"

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "core/check.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/repositories/currency_repository.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {

CurrencyEditor::CurrencyEditor(core::CurrencyRepository* currencies,
                               QObject* parent)
    : QObject(parent), currencies_(currencies) {
  core::Check(currencies_ != nullptr,
              "CurrencyEditor needs a CurrencyRepository");
}

CurrencyEditor* CurrencyEditor::create(QQmlEngine* qml_engine,
                                       QJSEngine* /*js_engine*/) {
  return SingletonInstance<CurrencyEditor>::ForEngine(qml_engine);
}

void CurrencyEditor::setCode(const QString& code) {
  const QString upper = code.trimmed().toUpper();
  if (upper != code_) {
    code_ = upper;
    emit changed();
  }
}

void CurrencyEditor::setMinorDigits(int digits) {
  if (digits != minor_digits_) {
    minor_digits_ = digits;
    emit changed();
  }
}

void CurrencyEditor::startNew() {
  code_.clear();
  minor_digits_ = kDefaultMinorDigits;
  error_text_.clear();
  emit changed();
}

bool CurrencyEditor::save() {
  const core::Status status =
      core::CurrencyCode::Parse(code_.toStdString())
          .and_then([&](core::CurrencyCode code) {
            return core::Currency::Create(code, minor_digits_);
          })
          .and_then([&](const core::Currency& currency) {
            return currencies_->Add(currency);
          });
  if (!status) {
    error_text_ = QString::fromStdString(status.error().message);
    emit changed();
    return false;
  }
  error_text_.clear();
  emit changed();
  emit saved(code_);
  return true;
}

}  // namespace mymoney::ui
