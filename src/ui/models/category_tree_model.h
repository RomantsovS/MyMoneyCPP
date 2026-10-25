#ifndef MYMONEY_UI_MODELS_CATEGORY_TREE_MODEL_H_
#define MYMONEY_UI_MODELS_CATEGORY_TREE_MODEL_H_

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

// The groups and categories of one kind for the Categories page (ADR-0018),
// as a flat list that reads like the tree: categories at the root first, then
// every group - also an empty one - followed by its categories (indented),
// each part sorted by name.
class CategoryTreeModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  // "expense" or "income"; changing it reloads the list.
  Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY loaded FINAL)
  Q_PROPERTY(QString errorText READ errorText NOTIFY loaded FINAL)

 public:
  // The roles QML reads, by name (see roleNames()). Qt passes roles as int.
  static constexpr int kCategoryIdRole = Qt::UserRole + 1;
  static constexpr int kNameRole = Qt::UserRole + 2;
  static constexpr int kIsGroupRole = Qt::UserRole + 3;
  static constexpr int kIndentedRole = Qt::UserRole + 4;  // inside a group

  // The repository is not owned and must outlive the model. Call reload() to
  // fill it.
  explicit CategoryTreeModel(core::CategoryRepository* categories,
                             QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static CategoryTreeModel* create(QQmlEngine* qml_engine,
                                   QJSEngine* js_engine);

  // Without a default argument (Google style); call rowCount({}).
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] QString kind() const;
  void setKind(const QString& kind);
  // Empty unless the last reload() failed.
  [[nodiscard]] QString errorText() const { return error_text_; }

  Q_INVOKABLE void reload();

 signals:
  void loaded();

 private:
  struct Row {
    core::CategoryId id;
    QString name;
    bool is_group;
    bool indented;
  };

  core::CategoryRepository* categories_;
  core::CategoryKind kind_ = core::CategoryKind::kExpense;
  std::vector<Row> rows_;
  QString error_text_;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_MODELS_CATEGORY_TREE_MODEL_H_
