#ifndef MYMONEY_UI_MODELS_CATEGORY_PICKER_MODEL_H_
#define MYMONEY_UI_MODELS_CATEGORY_PICKER_MODEL_H_

#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration>

#include "core/category.h"
#include "core/repositories/category_repository.h"
#include "core/strong_id.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The categories an expense or income may use, for the category picker
// (ADR-0017): only categories (never groups, ADR-0015) of one kind, sorted by
// their group and then by name. groupName is the ListView section: empty for
// categories at the root, which come first. filterText keeps the categories
// whose name or group contains it (case-insensitive).
class CategoryPickerModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY
                 loaded FINAL)
  Q_PROPERTY(QString errorText READ errorText NOTIFY loaded FINAL)

 public:
  // The roles QML reads, by name (see roleNames()). Qt passes roles as int.
  static constexpr int kCategoryIdRole = Qt::UserRole + 1;
  static constexpr int kNameRole = Qt::UserRole + 2;
  static constexpr int kGroupNameRole = Qt::UserRole + 3;

  // The repository is not owned and must outlive the model. Call load() to
  // fill it.
  explicit CategoryPickerModel(core::CategoryRepository* categories,
                               QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static CategoryPickerModel* create(QQmlEngine* qml_engine,
                                     QJSEngine* js_engine);

  // Without a default argument (Google style); call rowCount({}).
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] QString filterText() const { return filter_text_; }
  void setFilterText(const QString& text);
  // Empty unless the last load() failed.
  [[nodiscard]] QString errorText() const { return error_text_; }

  // Reads the categories for an operation of this type ("expense" or
  // "income") and clears the filter.
  Q_INVOKABLE void load(const QString& operation_type);

 signals:
  void loaded();

 private:
  struct Row {
    core::CategoryId id;
    QString name;
    QString group_name;  // empty at the root
  };

  // Shows the rows of all_rows_ that pass the filter.
  void ApplyFilter();

  core::CategoryRepository* categories_;
  std::vector<Row> all_rows_;
  std::vector<Row> rows_;  // the shown rows
  QString filter_text_;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_MODELS_CATEGORY_PICKER_MODEL_H_
