#include "ui/models/category_picker_model.h"

#include <algorithm>
#include <cstddef>
#include <map>
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
#include "core/strong_id.h"
#include "ui/singleton_instance.h"

namespace mymoney::ui {

CategoryPickerModel::CategoryPickerModel(core::CategoryRepository* categories,
                                         QObject* parent)
    : QAbstractListModel(parent), categories_(categories) {
  core::Check(categories_ != nullptr,
              "CategoryPickerModel needs a CategoryRepository");
}

CategoryPickerModel* CategoryPickerModel::create(QQmlEngine* qml_engine,
                                                 QJSEngine* /*js_engine*/) {
  return SingletonInstance<CategoryPickerModel>::ForEngine(qml_engine);
}

int CategoryPickerModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QVariant CategoryPickerModel::data(const QModelIndex& index, int role) const {
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
    case kGroupNameRole:
      return row.group_name;
    default:
      return {};
  }
}

QHash<int, QByteArray> CategoryPickerModel::roleNames() const {
  return {{kCategoryIdRole, "categoryId"},
          {kNameRole, "name"},
          {kGroupNameRole, "groupName"}};
}

void CategoryPickerModel::setFilterText(const QString& text) {
  if (text != filter_text_) {
    filter_text_ = text;
    ApplyFilter();
  }
}

void CategoryPickerModel::load(const QString& operation_type) {
  const core::CategoryKind kind = operation_type == u"income"
                                      ? core::CategoryKind::kIncome
                                      : core::CategoryKind::kExpense;
  core::Result<std::vector<core::Category>> categories = categories_->List();
  all_rows_.clear();
  if (categories) {
    std::map<core::CategoryId, QString> group_names;
    for (const core::Category& category : *categories) {
      if (category.is_group) {
        group_names.emplace(category.id, QString::fromStdString(category.name));
      }
    }
    for (const core::Category& category : *categories) {
      if (category.is_group || category.kind != kind) {
        continue;
      }
      all_rows_.push_back(
          {.id = category.id,
           .name = QString::fromStdString(category.name),
           .group_name = category.parent_id.has_value()
                             ? group_names.at(*category.parent_id)
                             : QString()});
    }
    // By group, then by name - in the user's language ("Ё" after "Е").
    std::ranges::sort(all_rows_, [](const Row& left, const Row& right) {
      if (const int by_group =
              QString::localeAwareCompare(left.group_name, right.group_name);
          by_group != 0) {
        return by_group < 0;
      }
      return QString::localeAwareCompare(left.name, right.name) < 0;
    });
    error_text_.clear();
  } else {
    error_text_ = QString::fromStdString(std::move(categories).error().message);
  }
  filter_text_.clear();
  ApplyFilter();
}

void CategoryPickerModel::ApplyFilter() {
  beginResetModel();
  rows_.clear();
  const QString filter = filter_text_.trimmed();
  for (const Row& row : all_rows_) {
    if (filter.isEmpty() || row.name.contains(filter, Qt::CaseInsensitive) ||
        row.group_name.contains(filter, Qt::CaseInsensitive)) {
      rows_.push_back(row);
    }
  }
  endResetModel();
  emit loaded();
}

}  // namespace mymoney::ui
