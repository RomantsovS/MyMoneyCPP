#include "ui/controllers/account_editor.h"

#include <expected>
#include <optional>
#include <utility>

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QtGlobal>

#include "core/account.h"
#include "core/check.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/repositories/account_repository.h"
#include "core/services/account_service.h"
#include "core/strong_id.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {

AccountEditor::AccountEditor(core::AccountRepository* accounts,
                             core::AccountService* service, QObject* parent)
    : QObject(parent), accounts_(accounts), service_(service) {
  core::Check(accounts_ != nullptr && service_ != nullptr,
              "AccountEditor needs an AccountRepository and AccountService");
}

AccountEditor* AccountEditor::create(QQmlEngine* qml_engine,
                                     QJSEngine* /*js_engine*/) {
  return SingletonInstance<AccountEditor>::ForEngine(qml_engine);
}

void AccountEditor::setName(const QString& name) {
  if (name != name_) {
    name_ = name;
    emit changed();
  }
}

void AccountEditor::setCurrencyCode(const QString& code) {
  if (code != currency_code_) {
    currency_code_ = code;
    emit changed();
  }
}

void AccountEditor::setArchived(bool archived) {
  if (archived != archived_) {
    archived_ = archived;
    emit changed();
  }
}

bool AccountEditor::valid() const {
  return !name_.trimmed().isEmpty() &&
         (account_.has_value() || !currency_code_.isEmpty());
}

void AccountEditor::startNew() {
  account_.reset();
  name_.clear();
  currency_code_.clear();
  archived_ = false;
  error_text_.clear();
  emit changed();
}

bool AccountEditor::load(qint64 account_id) {
  startNew();
  core::Result<core::Account> account =
      accounts_->Get(core::AccountId(account_id));
  if (!account) {
    error_text_ = QString::fromStdString(account.error().message);
    emit changed();
    return false;
  }
  name_ = QString::fromStdString(account->name);
  currency_code_ = QString::fromUtf8(account->currency.code());
  archived_ = account->archived;
  account_ = std::move(*account);
  emit changed();
  return true;
}

bool AccountEditor::save() {
  const QString name = name_.trimmed();
  core::Result<core::Account> result =
      account_.has_value() ? SaveExisting(*account_, name) : SaveNew(name);
  if (!result) {
    error_text_ = QString::fromStdString(result.error().message);
    emit changed();
    return false;
  }
  account_ = std::move(*result);
  name_ = name;
  error_text_.clear();
  emit changed();
  emit saved();
  return true;
}

core::Result<core::Account> AccountEditor::SaveNew(const QString& name) {
  return core::CurrencyCode::Parse(currency_code_.toStdString())
      .and_then([&](core::CurrencyCode currency) {
        return service_->Create(
            {.name = name.toStdString(), .currency = currency});
      });
}

core::Result<core::Account> AccountEditor::SaveExisting(
    const core::Account& stored, const QString& name) {
  // Two steps, not one transaction: if archiving fails, the new name stays -
  // the form shows the error and the user can try again.
  return service_->Rename(stored.id, name.toStdString())
      .and_then([&](core::Account account) -> core::Result<core::Account> {
        if (archived_ == account.archived) {
          return account;
        }
        const core::Status status = archived_ ? service_->Archive(account.id)
                                              : service_->Unarchive(account.id);
        if (!status) {
          return std::unexpected(status.error());
        }
        account.archived = archived_;
        return account;
      });
}

}  // namespace mymoney::ui
