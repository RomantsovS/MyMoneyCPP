#ifndef MYMONEY_UI_MODELS_JOURNAL_MODEL_H_
#define MYMONEY_UI_MODELS_JOURNAL_MODEL_H_

#include <chrono>
#include <functional>
#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QtGlobal>
#include <QtQmlIntegration>

#include "core/repositories/account_repository.h"
#include "core/repositories/category_repository.h"
#include "core/repositories/currency_repository.h"
#include "core/repositories/operation_repository.h"
#include "core/strong_id.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The Journal tab (ADR-0019): the operations of one month, newest first, with
// a day text for the ListView sections. Filters: an account, a category or
// group, and a comment text. While a comment text is set the month is
// ignored: the whole history is searched, at most kSearchLimit results. The
// filters run in SQL (OperationFilter); every change reloads the model.
class JournalModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(int year READ year NOTIFY loaded FINAL)
  Q_PROPERTY(int month READ month NOTIFY loaded FINAL)  // 1..12
  Q_PROPERTY(QString monthTitle READ monthTitle NOTIFY loaded FINAL)
  // -1: any account / any category.
  Q_PROPERTY(
      qint64 accountId READ accountId WRITE setAccountId NOTIFY loaded FINAL)
  Q_PROPERTY(QString accountName READ accountName NOTIFY loaded FINAL)
  Q_PROPERTY(
      qint64 categoryId READ categoryId WRITE setCategoryId NOTIFY loaded FINAL)
  Q_PROPERTY(QString categoryName READ categoryName NOTIFY loaded FINAL)
  Q_PROPERTY(QString commentText READ commentText WRITE setCommentText NOTIFY
                 loaded FINAL)
  // A comment text is set: the whole history is searched.
  Q_PROPERTY(bool searching READ searching NOTIFY loaded FINAL)
  // The search found more than kSearchLimit operations; the newest are shown.
  Q_PROPERTY(bool truncated READ truncated NOTIFY loaded FINAL)
  Q_PROPERTY(int searchLimit READ searchLimit CONSTANT FINAL)
  // For the filter pickers: maps {id, name}, starting with "any" (-1).
  // Accounts include archived ones; categories include groups (a group
  // filters all of its categories).
  Q_PROPERTY(
      QVariantList accountChoices READ accountChoices NOTIFY loaded FINAL)
  Q_PROPERTY(
      QVariantList categoryChoices READ categoryChoices NOTIFY loaded FINAL)
  Q_PROPERTY(QString errorText READ errorText NOTIFY loaded FINAL)

 public:
  static constexpr int kSearchLimit = 200;

  // The roles QML reads, by name (see roleNames()). Qt passes roles as int.
  static constexpr int kOperationIdRole = Qt::UserRole + 1;
  static constexpr int kDayTextRole = Qt::UserRole + 2;  // the section
  static constexpr int kTitleRole = Qt::UserRole + 3;    // category or a → b
  static constexpr int kAccountRole = Qt::UserRole + 4;  // empty: transfer
  static constexpr int kAmountTextRole = Qt::UserRole + 5;
  static constexpr int kKindRole = Qt::UserRole + 6;  // expense/income/...
  static constexpr int kCommentRole = Qt::UserRole + 7;

  using TodayFunction = std::function<std::chrono::year_month_day()>;

  // The repositories are not owned and must outlive the model. `today` gives
  // the current date: the first month shown, "Today" and "Yesterday".
  // Call reload() to fill the model.
  JournalModel(core::AccountRepository* accounts,
               core::CategoryRepository* categories,
               core::CurrencyRepository* currencies,
               core::OperationRepository* operations, TodayFunction today,
               QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static JournalModel* create(QQmlEngine* qml_engine, QJSEngine* js_engine);

  // Without a default argument (Google style); call rowCount({}).
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] int year() const;
  [[nodiscard]] int month() const;
  [[nodiscard]] QString monthTitle() const;
  [[nodiscard]] qint64 accountId() const { return account_id_; }
  void setAccountId(qint64 account_id);
  [[nodiscard]] QString accountName() const;
  [[nodiscard]] qint64 categoryId() const { return category_id_; }
  void setCategoryId(qint64 category_id);
  [[nodiscard]] QString categoryName() const;
  [[nodiscard]] QString commentText() const { return comment_text_; }
  void setCommentText(const QString& text);
  [[nodiscard]] bool searching() const {
    return !comment_text_.trimmed().isEmpty();
  }
  [[nodiscard]] bool truncated() const { return truncated_; }
  // A Q_PROPERTY getter must be a member function, so it cannot be static.
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  [[nodiscard]] int searchLimit() const { return kSearchLimit; }
  [[nodiscard]] QVariantList accountChoices() const { return account_choices_; }
  [[nodiscard]] QVariantList categoryChoices() const {
    return category_choices_;
  }
  // Empty unless the last reload() failed.
  [[nodiscard]] QString errorText() const { return error_text_; }

  Q_INVOKABLE void previousMonth();
  Q_INVOKABLE void nextMonth();
  // Reads the operations again (after an operation, account or category
  // changed) and the names behind them.
  Q_INVOKABLE void reload();

 signals:
  void loaded();

 private:
  struct Row {
    core::OperationId id;
    QString day_text;
    QString title;
    QString account;
    QString amount_text;
    QString kind;
    QString comment;
  };

  // The day header: "Today", "Yesterday" or e.g. "Fri, 23 Oct" (with the
  // year when it is not the current one).
  [[nodiscard]] QString DayText(std::chrono::year_month_day date) const;
  // The query for the current month or search, with the filters.
  [[nodiscard]] core::OperationFilter Filter() const;

  core::AccountRepository* accounts_;
  core::CategoryRepository* categories_;
  core::CurrencyRepository* currencies_;
  core::OperationRepository* operations_;
  TodayFunction today_;

  std::chrono::year_month month_;  // the month shown
  qint64 account_id_ = -1;
  qint64 category_id_ = -1;
  QString comment_text_;
  bool truncated_ = false;
  std::vector<Row> rows_;
  QVariantList account_choices_;
  QVariantList category_choices_;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_MODELS_JOURNAL_MODEL_H_
