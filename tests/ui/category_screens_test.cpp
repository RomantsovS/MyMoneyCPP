#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include <gtest/gtest.h>

#include "core/category.h"
#include "core/error.h"
#include "ui/controllers/category_editor.h"
#include "ui/models/category_tree_model.h"
#include "ui/qt_print.h"
#include "ui/ui_test_base.h"

namespace mymoney::ui::test {
namespace {

// u"..."_s is a QString.
using Qt::Literals::StringLiterals::operator""_s;

using core::CategoryKind;

class CategoryTreeModelTest : public UiTest {
 protected:
  // "name", with "  " in front of indented rows and "/" after groups.
  [[nodiscard]] QString Row(int row) const {
    const QModelIndex index = model_.index(row);
    QString text = model_.data(index, CategoryTreeModel::kNameRole).toString();
    if (model_.data(index, CategoryTreeModel::kIsGroupRole).toBool()) {
      text += u'/';
    }
    if (model_.data(index, CategoryTreeModel::kIndentedRole).toBool()) {
      text.prepend(u"  "_s);
    }
    return text;
  }

  CategoryTreeModel model_{services_.categories()};
  QAbstractItemModelTester tester_{
      &model_, QAbstractItemModelTester::FailureReportingMode::Fatal};
};

TEST_F(CategoryTreeModelTest, ReadsLikeTheTree) {
  const core::Category food = AddGroup("Food", CategoryKind::kExpense);
  AddCategory("Restaurants", CategoryKind::kExpense, food.id);
  AddCategory("Groceries", CategoryKind::kExpense, food.id);
  AddGroup("Car", CategoryKind::kExpense);  // empty, still listed
  AddCategory("Taxi", CategoryKind::kExpense);
  AddCategory("Salary", CategoryKind::kIncome);

  model_.reload();

  // Root categories first, then each group with its categories; by name.
  ASSERT_EQ(model_.rowCount({}), 5);
  EXPECT_EQ(Row(0), u"Taxi"_s);
  EXPECT_EQ(Row(1), u"Car/"_s);
  EXPECT_EQ(Row(2), u"Food/"_s);
  EXPECT_EQ(Row(3), u"  Groceries"_s);
  EXPECT_EQ(Row(4), u"  Restaurants"_s);
}

TEST_F(CategoryTreeModelTest, ShowsOneKindAtATime) {
  AddCategory("Taxi", CategoryKind::kExpense);
  AddCategory("Salary", CategoryKind::kIncome);
  model_.reload();
  const QSignalSpy loaded(&model_, &CategoryTreeModel::loaded);

  model_.setKind(u"income"_s);

  EXPECT_EQ(model_.kind(), u"income"_s);
  EXPECT_EQ(loaded.count(), 1);  // switching the kind reloads
  ASSERT_EQ(model_.rowCount({}), 1);
  EXPECT_EQ(Row(0), u"Salary"_s);
}

class CategoryEditorTest : public UiTest {
 protected:
  CategoryEditor editor_{services_.categories(), services_.category_service()};
  QSignalSpy saved_{&editor_, &CategoryEditor::saved};
};

TEST_F(CategoryEditorTest, AddsAGroupAndACategoryInIt) {
  editor_.startNewGroup(u"expense"_s);
  EXPECT_TRUE(editor_.isGroup());
  EXPECT_EQ(editor_.summary(), u"Expense group"_s);
  editor_.setName(u" Food "_s);
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();
  const qint64 food = editor_.savedId();

  editor_.startNewCategory(u"expense"_s);
  EXPECT_FALSE(editor_.isGroup());
  // The group picker: "No group" first, then the groups of this kind.
  const QVariantList groups = editor_.groups();
  ASSERT_EQ(groups.size(), 2);
  EXPECT_EQ(groups.at(0).toMap().value(u"categoryId"_s).toLongLong(), -1);
  EXPECT_EQ(groups.at(1).toMap().value(u"name"_s).toString(), u"Food"_s);
  editor_.setName(u"Groceries"_s);
  editor_.setGroupId(food);
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  ASSERT_EQ(saved_.count(), 2);
  EXPECT_EQ(saved_.at(1).at(0).toLongLong(), editor_.savedId());
  const core::Category groceries =
      services_.categories()->GetByName("Groceries").value();
  EXPECT_EQ(groceries.parent_id, core::CategoryId(food));
  EXPECT_EQ(services_.categories()->GetByName("Food")->name, "Food");
  EXPECT_EQ(editor_.summary(), u"Expense category in Food"_s);
}

TEST_F(CategoryEditorTest, AddsACategoryAtTheRoot) {
  editor_.startNewCategory(u"income"_s);
  EXPECT_EQ(editor_.groupId(), -1);
  editor_.setName(u"Salary"_s);

  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  const core::Category salary =
      services_.categories()->GetByName("Salary").value();
  EXPECT_EQ(salary.kind, CategoryKind::kIncome);
  EXPECT_EQ(salary.parent_id, std::nullopt);
}

TEST_F(CategoryEditorTest, RenamesAnExistingOne) {
  const core::Category food = AddGroup("Food", CategoryKind::kExpense);
  const core::Category cafe =
      AddCategory("Cafe", CategoryKind::kExpense, food.id);

  ASSERT_TRUE(editor_.load(cafe.id.value()));
  EXPECT_FALSE(editor_.isNew());
  EXPECT_EQ(editor_.name(), u"Cafe"_s);
  editor_.setName(u"Restaurants"_s);
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  const core::Category stored = services_.categories()->Get(cafe.id).value();
  EXPECT_EQ(stored.name, "Restaurants");
  EXPECT_EQ(stored.parent_id, food.id);  // stays in its group
}

TEST_F(CategoryEditorTest, ShowsTheCoresErrors) {
  AddCategory("Food", CategoryKind::kExpense);
  editor_.startNewGroup(u"income"_s);

  editor_.setName(u"  "_s);
  EXPECT_FALSE(editor_.valid());
  editor_.setName(u"Food"_s);  // names are unique across kinds
  EXPECT_FALSE(editor_.save());
  EXPECT_FALSE(editor_.errorText().isEmpty());
  EXPECT_EQ(saved_.count(), 0);

  EXPECT_FALSE(editor_.load(999));
  EXPECT_FALSE(editor_.errorText().isEmpty());
}

}  // namespace
}  // namespace mymoney::ui::test
