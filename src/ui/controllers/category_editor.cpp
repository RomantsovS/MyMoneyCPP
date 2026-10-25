#include "ui/controllers/category_editor.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtGlobal>

#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"
#include "core/services/category_service.h"
#include "core/strong_id.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {
namespace {

core::CategoryKind KindFromText(const QString& kind) {
  return kind == u"income" ? core::CategoryKind::kIncome
                           : core::CategoryKind::kExpense;
}

QVariantMap GroupEntry(qint64 id, const QString& name) {
  return {{QStringLiteral("categoryId"), QVariant::fromValue(id)},
          {QStringLiteral("name"), name}};
}

}  // namespace

CategoryEditor::CategoryEditor(core::CategoryRepository* categories,
                               core::CategoryService* service, QObject* parent)
    : QObject(parent), categories_(categories), service_(service) {
  core::Check(categories_ != nullptr && service_ != nullptr,
              "CategoryEditor needs a CategoryRepository and CategoryService");
}

CategoryEditor* CategoryEditor::create(QQmlEngine* qml_engine,
                                       QJSEngine* /*js_engine*/) {
  return SingletonInstance<CategoryEditor>::ForEngine(qml_engine);
}

bool CategoryEditor::isGroup() const {
  return category_.has_value() ? category_->is_group : new_is_group_;
}

void CategoryEditor::setName(const QString& name) {
  if (name != name_) {
    name_ = name;
    emit changed();
  }
}

void CategoryEditor::setGroupId(qint64 group_id) {
  if (group_id != group_id_) {
    group_id_ = group_id;
    emit changed();
  }
}

QString CategoryEditor::summary() const {
  const core::CategoryKind kind =
      category_.has_value() ? category_->kind : kind_;
  const bool expense = kind == core::CategoryKind::kExpense;
  if (isGroup()) {
    return expense ? tr("Expense group") : tr("Income group");
  }
  const std::optional<core::CategoryId> group =
      category_.has_value() ? category_->parent_id : std::nullopt;
  if (group.has_value()) {
    const core::Result<core::Category> parent = categories_->Get(*group);
    if (parent) {
      const QString name = QString::fromStdString(parent->name);
      return expense ? tr("Expense category in %1").arg(name)
                     : tr("Income category in %1").arg(name);
    }
  }
  return expense ? tr("Expense category") : tr("Income category");
}

void CategoryEditor::startNewGroup(const QString& kind) {
  Reset(KindFromText(kind), /*is_group=*/true);
}

void CategoryEditor::startNewCategory(const QString& kind) {
  Reset(KindFromText(kind), /*is_group=*/false);
}

bool CategoryEditor::load(qint64 category_id) {
  Reset(core::CategoryKind::kExpense, /*is_group=*/false);
  core::Result<core::Category> category =
      categories_->Get(core::CategoryId(category_id));
  if (!category) {
    error_text_ = QString::fromStdString(category.error().message);
    emit changed();
    return false;
  }
  name_ = QString::fromStdString(category->name);
  category_ = std::move(*category);
  emit changed();
  return true;
}

bool CategoryEditor::save() {
  const QString name = name_.trimmed();
  const core::Result<core::Category> result =
      category_.has_value()
          ? service_->Rename(category_->id, name.toStdString())
          : service_->Create(
                {.name = name.toStdString(),
                 .kind = kind_,
                 .parent_id = !new_is_group_ && group_id_ >= 0
                                  ? std::optional(core::CategoryId(group_id_))
                                  : std::nullopt,
                 .is_group = new_is_group_});
  if (!result) {
    error_text_ = QString::fromStdString(result.error().message);
    emit changed();
    return false;
  }
  category_ = *result;  // the form now shows the stored item
  name_ = name;
  saved_id_ = result->id.value();
  error_text_.clear();
  emit changed();
  emit saved(saved_id_);
  return true;
}

void CategoryEditor::Reset(core::CategoryKind kind, bool is_group) {
  category_.reset();
  kind_ = kind;
  new_is_group_ = is_group;
  name_.clear();
  group_id_ = -1;
  error_text_.clear();
  saved_id_ = -1;
  LoadGroups();
  emit changed();
}

void CategoryEditor::LoadGroups() {
  groups_.clear();
  groups_.append(GroupEntry(-1, tr("No group")));
  core::Result<std::vector<core::Category>> all = categories_->List();
  if (!all) {
    return;  // the category can still be created at the root
  }
  std::vector<core::Category> groups;
  std::ranges::copy_if(*all, std::back_inserter(groups),
                       [&](const core::Category& category) {
                         return category.is_group && category.kind == kind_;
                       });
  std::ranges::sort(groups, [](const core::Category& left,
                               const core::Category& right) {
    return QString::localeAwareCompare(QString::fromStdString(left.name),
                                       QString::fromStdString(right.name)) < 0;
  });
  for (const core::Category& group : groups) {
    groups_.append(
        GroupEntry(group.id.value(), QString::fromStdString(group.name)));
  }
}

}  // namespace mymoney::ui
