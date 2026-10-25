#ifndef MYMONEY_UI_CONTROLLERS_CATEGORY_EDITOR_H_
#define MYMONEY_UI_CONTROLLERS_CATEGORY_EDITOR_H_

#include <optional>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtGlobal>
#include <QtQmlIntegration>

#include "core/category.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"
#include "core/services/category_service.h"

class QJSEngine;
class QQmlEngine;

namespace mymoney::ui {

// The state behind the category form (ADR-0018): a new group (a name), a new
// category (a name and an optional group of the same kind), or an existing
// one, which can only be renamed - its kind and group stay. The page calls
// startNewGroup(), startNewCategory() or load() when it opens and save() when
// the user confirms.
class CategoryEditor : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(bool isNew READ isNew NOTIFY changed FINAL)
  Q_PROPERTY(bool isGroup READ isGroup NOTIFY changed FINAL)
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed FINAL)
  // The group of a new category; -1 for "no group" (at the root).
  Q_PROPERTY(qint64 groupId READ groupId WRITE setGroupId NOTIFY changed FINAL)
  // For the group picker of a new category: maps {categoryId, name}, first
  // "No group" (-1), then the groups of the kind by name.
  Q_PROPERTY(QVariantList groups READ groups NOTIFY changed FINAL)
  // What is edited, e.g. "Expense category in Food" or "Income group".
  Q_PROPERTY(QString summary READ summary NOTIFY changed FINAL)
  Q_PROPERTY(bool valid READ valid NOTIFY changed FINAL)
  // Empty unless the last load() or save() failed.
  Q_PROPERTY(QString errorText READ errorText NOTIFY changed FINAL)
  // After a successful save(): the id of the saved category or group.
  Q_PROPERTY(qint64 savedId READ savedId NOTIFY changed FINAL)

 public:
  // The repository and the service are not owned and must outlive the editor.
  CategoryEditor(core::CategoryRepository* categories,
                 core::CategoryService* service, QObject* parent = nullptr);

  // For the QML engine (see SingletonInstance).
  static CategoryEditor* create(QQmlEngine* qml_engine, QJSEngine* js_engine);

  [[nodiscard]] bool isNew() const { return !category_.has_value(); }
  [[nodiscard]] bool isGroup() const;
  [[nodiscard]] QString name() const { return name_; }
  void setName(const QString& name);
  [[nodiscard]] qint64 groupId() const { return group_id_; }
  void setGroupId(qint64 group_id);
  [[nodiscard]] QVariantList groups() const { return groups_; }
  [[nodiscard]] QString summary() const;
  [[nodiscard]] bool valid() const { return !name_.trimmed().isEmpty(); }
  [[nodiscard]] QString errorText() const { return error_text_; }
  [[nodiscard]] qint64 savedId() const { return saved_id_; }

  // An empty form for a new group or category of `kind` ("expense" or
  // "income").
  Q_INVOKABLE void startNewGroup(const QString& kind);
  Q_INVOKABLE void startNewCategory(const QString& kind);
  // The form for an existing category or group; false (and errorText) if it
  // fails.
  Q_INVOKABLE bool load(qint64 category_id);
  // Creates it, or renames an existing one. The name is trimmed first. True
  // and saved(id) on success; false and errorText otherwise.
  Q_INVOKABLE bool save();

 signals:
  void changed();
  void saved(qint64 category_id);

 private:
  // Clears the form for a new item.
  void Reset(core::CategoryKind kind, bool is_group);
  void LoadGroups();

  core::CategoryRepository* categories_;
  core::CategoryService* service_;

  std::optional<core::Category> category_;  // as stored; nullopt for a new one
  core::CategoryKind kind_ = core::CategoryKind::kExpense;  // of a new one
  bool new_is_group_ = false;
  QString name_;
  qint64 group_id_ = -1;
  QVariantList groups_;
  QString error_text_;
  qint64 saved_id_ = -1;
};

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_CONTROLLERS_CATEGORY_EDITOR_H_
