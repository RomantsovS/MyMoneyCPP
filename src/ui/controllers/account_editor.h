#ifndef MYMONEY_UI_CONTROLLERS_ACCOUNT_EDITOR_H_
#define MYMONEY_UI_CONTROLLERS_ACCOUNT_EDITOR_H_

#include <optional>

#include <QObject>
#include <QString>
#include <QtGlobal>
#include <QtQmlIntegration>

#include "core/account.h"
#include "core/error.h"
#include "core/repositories/account_repository.h"
#include "core/services/account_service.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The state behind the account form (ADR-0016): a new account (name and
// currency) or an existing one (name and archived; the currency never
// changes). The page calls startNew() or load() when it opens and save() when
// the user confirms; QML binds to the properties.
class AccountEditor : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(bool isNew READ isNew NOTIFY changed FINAL)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed FINAL)
  Q_PROPERTY(QString currencyCode READ currencyCode WRITE setCurrencyCode NOTIFY
                 changed FINAL)
  Q_PROPERTY(bool archived READ archived WRITE setArchived NOTIFY changed FINAL)
  // The form may be saved: a name, and a currency for a new account. The core
  // checks everything else on save().
  Q_PROPERTY(bool valid READ valid NOTIFY changed FINAL)
  // Empty unless the last load() or save() failed.
  Q_PROPERTY(QString errorText READ errorText NOTIFY changed FINAL)

 public:
  // The repository and the service are not owned and must outlive the editor.
  AccountEditor(core::AccountRepository* accounts,
                core::AccountService* service, QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static AccountEditor* create(QQmlEngine* qml_engine, QJSEngine* js_engine);

  [[nodiscard]] bool isNew() const { return !account_.has_value(); }
  [[nodiscard]] QString name() const { return name_; }
  void setName(const QString& name);
  [[nodiscard]] QString currencyCode() const { return currency_code_; }
  void setCurrencyCode(const QString& code);
  [[nodiscard]] bool archived() const { return archived_; }
  void setArchived(bool archived);
  [[nodiscard]] bool valid() const;
  [[nodiscard]] QString errorText() const { return error_text_; }

  // An empty form for a new account.
  Q_INVOKABLE void startNew();
  // The form for an existing account; false (and errorText) if it fails.
  Q_INVOKABLE bool load(qint64 account_id);
  // Creates or updates the account. The name is trimmed first (phone
  // keyboards like to add a space). True and saved() on success; false and
  // errorText otherwise.
  Q_INVOKABLE bool save();

 signals:
  void changed();
  void saved();

 private:
  [[nodiscard]] core::Result<core::Account> SaveNew(const QString& name);
  [[nodiscard]] core::Result<core::Account> SaveExisting(
      const core::Account& stored, const QString& name);

  core::AccountRepository* accounts_;
  core::AccountService* service_;

  std::optional<core::Account> account_;  // as stored; nullopt for a new one
  QString name_;
  QString currency_code_;
  bool archived_ = false;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_CONTROLLERS_ACCOUNT_EDITOR_H_
