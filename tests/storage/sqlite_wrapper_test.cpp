#include <cstdint>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "core/error.h"
#include "core/test_util.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"
#include "storage/sqlite/transaction.h"
#include "storage/storage.h"

namespace mymoney::storage::sqlite {
namespace {

class SqliteWrapperTest : public ::testing::Test {
 protected:
  SqliteWrapperTest() {
    db_.Execute("CREATE TABLE item (number INTEGER, text TEXT, flag INTEGER)")
        .value();
  }

  std::int64_t CountItems() {
    Statement count = db_.Prepare("SELECT COUNT(*) FROM item").value();
    EXPECT_TRUE(count.Step().value());
    return count.Column<std::int64_t>(0);
  }

  Database db_ = Database::Open(":memory:").value();
};

TEST_F(SqliteWrapperTest, BindsAndReadsEverySupportedType) {
  ASSERT_TRUE(db_.Run("INSERT INTO item VALUES (?1, ?2, ?3)", std::int64_t{42},
                      "text", true));
  ASSERT_TRUE(db_.Run("INSERT INTO item VALUES (?1, ?2, ?3)", std::nullopt,
                      std::optional<std::string>{}, false));

  Statement select =
      db_.Prepare("SELECT number, text, flag FROM item ORDER BY rowid").value();
  ASSERT_TRUE(select.Step().value());
  EXPECT_EQ(select.Column<std::int64_t>(0), 42);
  EXPECT_EQ(select.Column<std::string>(1), "text");
  EXPECT_TRUE(select.Column<bool>(2));
  ASSERT_TRUE(select.Step().value());
  EXPECT_EQ(select.Column<std::optional<std::int64_t>>(0), std::nullopt);
  EXPECT_EQ(select.Column<std::optional<std::string>>(1), std::nullopt);
  EXPECT_FALSE(select.Column<bool>(2));
  EXPECT_FALSE(select.Step().value());  // no more rows
}

TEST_F(SqliteWrapperTest, BoundTextIsNeverInterpretedAsSql) {
  // Bound parameters are data, not SQL: quotes and SQL keywords are harmless.
  const std::string tricky = "O'Reilly'); DROP TABLE item; -- \xE2\x82\xAC";
  ASSERT_TRUE(db_.Run("INSERT INTO item (text) VALUES (?1)", tricky));

  Statement select = db_.Prepare("SELECT text FROM item").value();
  ASSERT_TRUE(select.Step().value());
  EXPECT_EQ(select.Column<std::string>(0), tricky);
}

TEST_F(SqliteWrapperTest, InvalidSqlIsAStorageError) {
  const core::Result<Statement> statement = db_.Prepare("SELEKT 1");

  ASSERT_FALSE(statement.has_value());
  EXPECT_EQ(statement.error().code, core::ErrorCode::kStorage);
}

TEST_F(SqliteWrapperTest, PrepareRefusesMoreThanOneStatement) {
  const core::Result<Statement> statement =
      db_.Prepare("DELETE FROM item; DROP TABLE item");

  ASSERT_FALSE(statement.has_value());
  EXPECT_EQ(statement.error().code, core::ErrorCode::kStorage);
}

TEST_F(SqliteWrapperTest, TransactionWithoutCommitIsRolledBack) {
  {
    const Transaction transaction = Transaction::Begin(db_).value();
    ASSERT_TRUE(db_.Run("INSERT INTO item (number) VALUES (1)"));
    EXPECT_EQ(CountItems(), 1);
  }  // destroyed without Commit()

  EXPECT_EQ(CountItems(), 0);
}

TEST_F(SqliteWrapperTest, CommittedTransactionIsKept) {
  Transaction transaction = Transaction::Begin(db_).value();
  ASSERT_TRUE(db_.Run("INSERT INTO item (number) VALUES (1)"));
  ASSERT_TRUE(transaction.Commit());

  EXPECT_EQ(CountItems(), 1);
}

// The schema defends itself: these writes bypass all C++ validation.
TEST(SchemaTest, RejectsInvalidDataFromAnyWriter) {
  Database db = OpenStorage(":memory:").value();

  // CHECK: a currency code is three capital letters.
  EXPECT_FALSE(db.Run("INSERT INTO currency VALUES ('eur', 2)"));
  // STRICT: minor_digits must be an INTEGER.
  EXPECT_FALSE(db.Run("INSERT INTO currency VALUES ('EUR', 'two')"));
  // FOREIGN KEY: an account needs a known currency.
  EXPECT_FALSE(
      db.Run("INSERT INTO account (name, currency) VALUES ('A', 'XXX')"));
  // CHECK: no blank names.
  ASSERT_TRUE(db.Run("INSERT INTO currency VALUES ('EUR', 2)"));
  EXPECT_FALSE(
      db.Run("INSERT INTO account (name, currency) VALUES (' ', 'EUR')"));
  // CHECK: dates are ISO text.
  EXPECT_FALSE(db.Run(
      "INSERT INTO operation (date, type) VALUES ('18.10.2026', 'expense')"));
}

}  // namespace
}  // namespace mymoney::storage::sqlite
