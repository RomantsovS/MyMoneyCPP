#include "ui/models/category_tree_model.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QJSEngine>
#include <QModelIndex>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>

#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/repositories/category_repository.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {
namespace {

// By name, in the user's language ("Ё" after "Е").
void SortByName(std::vector<core::Category>& categories) {
  std::ranges::sort(categories, [](const core::Category& left,
                                   const core::Category& right) {
    return QString::localeAwareCompare(QString::fromStdString(left.name),
                                       QString::fromStdString(right.name)) < 0;
  });
}

}  // namespace

CategoryTreeModel::CategoryTreeModel(core::CategoryRepository* categories,
                                     QObject* parent)
    : QAbstractListModel(parent), categories_(categories) {
  core::Check(categories_ != nullptr,
              "CategoryTreeModel needs a CategoryRepository");
}

CategoryTreeModel* CategoryTreeModel::create(QQmlEngine* qml_engine,
                                             QJSEngine* /*js_engine*/) {
  return SingletonInstance<CategoryTreeModel>::ForEngine(qml_engine);
}

int CategoryTreeModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant CategoryTreeModel::data(const QModelIndex& index, int role) const {
  if (!checkIndex(index, CheckIndexOption::IndexIsValid |
                             CheckIndexOption::ParentIsInvalid)) {
    return {};
  }
  const Row& row = rows_.at(static_cast<std::size_t>(index.row()));
  switch (role) {
    case kCategoryIdRole:
      return QVariant::fromValue(static_cast<qint64>(row.id.value()));
    case Qt::DisplayRole:
    case kNameRole:
      return row.name;
    case kIsGroupRole:
      return row.is_group;
    case kIndentedRole:
      return row.indented;
    default:
      return {};
  }
}

QHash<int, QByteArray> CategoryTreeModel::roleNames() const {
  return {{kCategoryIdRole, "categoryId"},
          {kNameRole, "name"},
          {kIsGroupRole, "isGroup"},
          {kIndentedRole, "indented"}};
}

QString CategoryTreeModel::kind() const {
  return kind_ == core::CategoryKind::kIncome ? QStringLiteral("income")
                                              : QStringLiteral("expense");
}

void CategoryTreeModel::setKind(const QString& kind) {
  const core::CategoryKind new_kind = kind == u"income"
                                          ? core::CategoryKind::kIncome
                                          : core::CategoryKind::kExpense;
  if (new_kind != kind_) {
    kind_ = new_kind;
    reload();
  }
}

void CategoryTreeModel::reload() {
  core::Result<std::vector<core::Category>> all = categories_->List();
  beginResetModel();
  rows_.clear();
  if (all) {
    std::vector<core::Category> roots;   // categories at the root
    std::vector<core::Category> groups;  // groups, always at the root
    std::vector<core::Category> inside;  // categories inside a group
    for (core::Category& category : *all) {
      if (category.kind != kind_) {
        continue;
      }
      if (category.is_group) {
        groups.push_back(std::move(category));
      } else if (category.parent_id.has_value()) {
        inside.push_back(std::move(category));
      } else {
        roots.push_back(std::move(category));
      }
    }
    SortByName(roots);
    SortByName(groups);
    SortByName(inside);
    const auto add = [&](const core::Category& category, bool indented) {
      rows_.push_back({.id = category.id,
                       .name = QString::fromStdString(category.name),
                       .is_group = category.is_group,
                       .indented = indented});
    };
    for (const core::Category& category : roots) {
      add(category, false);
    }
    for (const core::Category& group : groups) {
      add(group, false);
      for (const core::Category& category : inside) {
        if (category.parent_id == group.id) {
          add(category, true);
        }
      }
    }
    error_text_.clear();
  } else {
    error_text_ = QString::fromStdString(std::move(all).error().message);
  }
  endResetModel();
  emit loaded();
}

}  // namespace mymoney::ui
