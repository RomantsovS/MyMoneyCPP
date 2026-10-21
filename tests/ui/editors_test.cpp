#include <QSignalSpy>
#include <QString>

#include <gtest/gtest.h>

#include "core/account.h"
#include "core/currency.h"
#include "core/currency_code.h"
#include "core/error.h"
#include "core/test_util.h"
#include "ui/controllers/account_editor.h"
#include "ui/controllers/currency_editor.h"
#include "ui/qt_print.h"
#include "ui/ui_test_base.h"

namespace mymoney::ui::test {
namespace {

// u"..."_s is a QString.
using Qt::Literals::StringLiterals::operator""_s;

using core::test::kEur;

class AccountEditorTest : public UiTest {
 protected:
  AccountEditorTest() { AddCurrency(kEur); }

  AccountEditor editor_{services_.accounts(), services_.account_service()};
  QSignalSpy saved_{&editor_, &AccountEditor::saved};
};

TEST_F(AccountEditorTest, CreatesAnAccount) {
  editor_.startNew();
  EXPECT_TRUE(editor_.isNew());
  EXPECT_FALSE(editor_.valid());

  // A phone keyboard adds a space after a word: the name is trimmed.
  editor_.setName(u"Card "_s);
  editor_.setCurrencyCode(u"EUR"_s);
  ASSERT_TRUE(editor_.valid());

  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();
  EXPECT_EQ(saved_.count(), 1);
  EXPECT_FALSE(editor_.isNew());  // the form now shows the stored account
  EXPECT_EQ(editor_.name(), u"Card"_s);
  const core::Result<core::Account> stored =
      services_.accounts()->GetByName("Card");
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(stored->currency, kEur);
}

TEST_F(AccountEditorTest, NewAccountNeedsNameAndCurrency) {
  editor_.startNew();
  editor_.setName(u"  "_s);
  editor_.setCurrencyCode(u"EUR"_s);
  EXPECT_FALSE(editor_.valid());

  editor_.setName(u"Card"_s);
  editor_.setCurrencyCode(QString());
  EXPECT_FALSE(editor_.valid());
}

TEST_F(AccountEditorTest, ShowsTheCoresErrorWhenSavingFails) {
  AddAccount("Card", kEur);
  editor_.startNew();
  editor_.setName(u"Card"_s);
  editor_.setCurrencyCode(u"EUR"_s);

  EXPECT_FALSE(editor_.save());
  EXPECT_FALSE(editor_.errorText().isEmpty());
  EXPECT_EQ(saved_.count(), 0);

  // An unknown currency is refused too.
  editor_.setName(u"Yen"_s);
  editor_.setCurrencyCode(u"JPY"_s);
  EXPECT_FALSE(editor_.save());
  EXPECT_EQ(saved_.count(), 0);
}

TEST_F(AccountEditorTest, RenamesAndArchivesAnExistingAccount) {
  const core::Account card = AddAccount("Card", kEur);

  ASSERT_TRUE(editor_.load(card.id.value()));
  EXPECT_FALSE(editor_.isNew());
  EXPECT_EQ(editor_.name(), u"Card"_s);
  EXPECT_EQ(editor_.currencyCode(), u"EUR"_s);
  EXPECT_FALSE(editor_.archived());

  editor_.setName(u"Visa"_s);
  editor_.setArchived(true);
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  EXPECT_EQ(saved_.count(), 1);
  const core::Result<core::Account> stored = services_.accounts()->Get(card.id);
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(stored->name, "Visa");
  EXPECT_TRUE(stored->archived);

  editor_.setArchived(false);
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();
  EXPECT_FALSE(services_.accounts()->Get(card.id)->archived);
}

TEST_F(AccountEditorTest, LoadingAnUnknownAccountFails) {
  EXPECT_FALSE(editor_.load(999));
  EXPECT_FALSE(editor_.errorText().isEmpty());
}

class CurrencyEditorTest : public UiTest {
 protected:
  CurrencyEditor editor_{services_.currencies()};
  QSignalSpy saved_{&editor_, &CurrencyEditor::saved};
};

TEST_F(CurrencyEditorTest, AddsACurrencyInUpperCase) {
  editor_.startNew();
  EXPECT_EQ(editor_.minorDigits(), CurrencyEditor::kDefaultMinorDigits);

  editor_.setCode(u"rub"_s);
  EXPECT_EQ(editor_.code(), u"RUB"_s);
  ASSERT_TRUE(editor_.valid());
  ASSERT_TRUE(editor_.save()) << editor_.errorText().toStdString();

  ASSERT_EQ(saved_.count(), 1);
  EXPECT_EQ(saved_.at(0).at(0).toString(), u"RUB"_s);
  const core::Result<core::Currency> stored =
      services_.currencies()->Get(core::CurrencyCode("RUB"));
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(stored->minor_digits(), 2);
}

TEST_F(CurrencyEditorTest, RefusesWhatTheCoreRefuses) {
  AddCurrency(kEur);
  editor_.startNew();

  editor_.setCode(u"EUR"_s);  // exists already
  EXPECT_FALSE(editor_.save());
  EXPECT_FALSE(editor_.errorText().isEmpty());

  editor_.setCode(u"GBP"_s);
  editor_.setMinorDigits(5);  // at most 4
  EXPECT_FALSE(editor_.save());

  editor_.setCode(u"E1"_s);
  EXPECT_FALSE(editor_.valid());
  EXPECT_EQ(saved_.count(), 0);
}

}  // namespace
}  // namespace mymoney::ui::test
