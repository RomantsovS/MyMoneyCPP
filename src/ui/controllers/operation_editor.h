#ifndef MYMONEY_UI_CONTROLLERS_OPERATION_EDITOR_H_
#define MYMONEY_UI_CONTROLLERS_OPERATION_EDITOR_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtGlobal>
#include <QtQmlIntegration>

#include "core/account.h"
#include "core/category.h"
#include "core/error.h"
#include "core/operation.h"
#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/services/operation_service.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The state behind the operation form (ADR-0017): one form for an expense, an
// income or a transfer. startNew() fills in the accounts of the latest
// operation of that type and today's date; QML binds to the properties and
// calls the choose...() and set...Date() functions; save() posts the
// operation through OperationService, which checks every rule again.
class OperationEditor : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  // "expense", "income" or "transfer" - the texts the CLI and the database
  // use. (A C++ enum would need names starting with an upper-case letter to
  // be usable in QML, which our naming rules do not allow.)
  Q_PROPERTY(QString type READ type WRITE setType NOTIFY changed FINAL)
  Q_PROPERTY(QString amountText READ amountText WRITE setAmountText NOTIFY
                 changed FINAL)
  // For a transfer between currencies: the amount the target receives.
  Q_PROPERTY(QString amountInText READ amountInText WRITE setAmountInText NOTIFY
                 changed FINAL)
  Q_PROPERTY(bool needsAmountIn READ needsAmountIn NOTIFY changed FINAL)
  // The account (for a transfer: the source); -1 and empty texts if none.
  Q_PROPERTY(qint64 accountId READ accountId NOTIFY changed FINAL)
  Q_PROPERTY(QString accountName READ accountName NOTIFY changed FINAL)
  Q_PROPERTY(QString currencyCode READ currencyCode NOTIFY changed FINAL)
  // The target account of a transfer.
  Q_PROPERTY(qint64 toAccountId READ toAccountId NOTIFY changed FINAL)
  Q_PROPERTY(QString toAccountName READ toAccountName NOTIFY changed FINAL)
  Q_PROPERTY(QString toCurrencyCode READ toCurrencyCode NOTIFY changed FINAL)
  // The category of an expense or income; -1 and "" if none is chosen yet.
  Q_PROPERTY(qint64 categoryId READ categoryId NOTIFY changed FINAL)
  Q_PROPERTY(QString categoryName READ categoryName NOTIFY changed FINAL)
  // Up to kRecentCategoryCount maps {categoryId, name}, newest first.
  Q_PROPERTY(
      QVariantList recentCategories READ recentCategories NOTIFY changed FINAL)
  // The date: shown with the device's locale, flagged when it is today or
  // yesterday, and as numbers for the calendar (month 1..12).
  Q_PROPERTY(QString dateText READ dateText NOTIFY changed FINAL)
  Q_PROPERTY(bool isToday READ isToday NOTIFY changed FINAL)
  Q_PROPERTY(bool isYesterday READ isYesterday NOTIFY changed FINAL)
  Q_PROPERTY(int year READ year NOTIFY changed FINAL)
  Q_PROPERTY(int month READ month NOTIFY changed FINAL)
  Q_PROPERTY(int day READ day NOTIFY changed FINAL)
  Q_PROPERTY(QString comment READ comment WRITE setComment NOTIFY changed FINAL)
  // Everything needed is filled in and the amounts can be read; the core
  // checks the rest on save().
  Q_PROPERTY(bool valid READ valid NOTIFY changed FINAL)
  // Empty unless the last save() failed.
  Q_PROPERTY(QString errorText READ errorText NOTIFY changed FINAL)
  // After a successful save(): e.g. "Expense 12.50 EUR saved".
  Q_PROPERTY(QString savedText READ savedText NOTIFY changed FINAL)

 public:
  static constexpr int kRecentCategoryCount = 6;

  using TodayFunction = std::function<std::chrono::year_month_day()>;

  // The repositories and the service are not owned and must outlive the
  // editor. `today` gives the current date (injected so tests are
  // repeatable).
  OperationEditor(core::AccountRepository* accounts,
                  core::CategoryRepository* categories,
                  core::CurrencyRepository* currencies,
                  core::OperationRepository* operations,
                  core::OperationService* service, TodayFunction today,
                  QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static OperationEditor* create(QQmlEngine* qml_engine, QJSEngine* js_engine);

  [[nodiscard]] QString type() const;
  // Pre-fills the new type's accounts unless the user already chose one
  // (then a transfer only gets a second account if it needs one); the
  // category is cleared and the recent categories follow the new type.
  // Unknown texts are ignored.
  void setType(const QString& type);
  [[nodiscard]] QString amountText() const { return amount_text_; }
  void setAmountText(const QString& text);
  [[nodiscard]] QString amountInText() const { return amount_in_text_; }
  void setAmountInText(const QString& text);
  [[nodiscard]] bool needsAmountIn() const;
  [[nodiscard]] qint64 accountId() const;
  [[nodiscard]] QString accountName() const;
  [[nodiscard]] QString currencyCode() const;
  [[nodiscard]] qint64 toAccountId() const;
  [[nodiscard]] QString toAccountName() const;
  [[nodiscard]] QString toCurrencyCode() const;
  [[nodiscard]] qint64 categoryId() const;
  [[nodiscard]] QString categoryName() const;
  [[nodiscard]] QVariantList recentCategories() const {
    return recent_categories_;
  }
  [[nodiscard]] QString dateText() const;
  [[nodiscard]] bool isToday() const { return date_ == today_(); }
  [[nodiscard]] bool isYesterday() const;
  [[nodiscard]] int year() const;
  [[nodiscard]] int month() const;
  [[nodiscard]] int day() const;
  [[nodiscard]] QString comment() const { return comment_; }
  void setComment(const QString& comment);
  [[nodiscard]] bool valid() const;
  [[nodiscard]] QString errorText() const { return error_text_; }
  [[nodiscard]] QString savedText() const { return saved_text_; }

  // An empty expense with the pre-filled account and today's date.
  Q_INVOKABLE void startNew();
  // The account, the target account of a transfer, the category. Unknown
  // ids are ignored (the pickers only offer existing ones).
  Q_INVOKABLE void chooseAccount(qint64 account_id);
  Q_INVOKABLE void chooseToAccount(qint64 account_id);
  Q_INVOKABLE void chooseCategory(qint64 category_id);
  Q_INVOKABLE void setToday();
  Q_INVOKABLE void setYesterday();
  // From the calendar, as numbers (month 1..12): a JavaScript Date would be a
  // time in a time zone. An invalid date is ignored.
  Q_INVOKABLE void setDate(int year, int month, int day);
  // Posts the operation. True, savedText and saved() on success; false and
  // errorText otherwise.
  Q_INVOKABLE bool save();

 signals:
  void changed();
  void saved();

 private:
  // An account with its currency's number of decimal places, which the
  // amount text is read with.
  struct ChosenAccount {
    core::Account account;
    int minor_digits;
  };

  [[nodiscard]] std::optional<ChosenAccount> LoadAccount(
      core::AccountId id) const;
  // The first active account that is not `except`.
  [[nodiscard]] std::optional<ChosenAccount> FirstActiveAccount(
      std::optional<core::AccountId> except) const;
  // The accounts of the latest operation of the current type, if they are
  // still active; otherwise the first active account(s).
  void ApplyDefaultAccounts();
  // A transfer's target: kept if set and different from the source,
  // otherwise the first other active account.
  void FillToAccount();
  void LoadRecentCategories();
  [[nodiscard]] core::Result<core::OperationDraft> BuildDraft() const;

  core::AccountRepository* accounts_;
  core::CategoryRepository* categories_;
  core::CurrencyRepository* currencies_;
  core::OperationRepository* operations_;
  core::OperationService* service_;
  TodayFunction today_;

  core::OperationType type_ = core::OperationType::kExpense;
  QString amount_text_;
  QString amount_in_text_;
  std::optional<ChosenAccount> account_;
  std::optional<ChosenAccount> to_account_;
  std::optional<core::Category> category_;
  QVariantList recent_categories_;
  std::chrono::year_month_day date_{};  // set to today by the constructor
  bool accounts_chosen_ = false;  // the user picked an account in this form
  QString comment_;
  QString error_text_;
  QString saved_text_;
};

// The minor units of a positive amount typed for an account with
// `minor_digits` decimal places, or nullopt if the text is not one.
[[nodiscard]] std::optional<std::int64_t> ParsePositiveAmount(
    const QString& text, int minor_digits);

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_CONTROLLERS_OPERATION_EDITOR_H_
