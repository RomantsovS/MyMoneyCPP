#include "storage/migrations.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include <gtest/gtest.h>

#include "core/category.h"
#include "core/error.h"
#include "core/operation.h"
#include "core/strong_id.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"
#include "storage/sqlite_category_repository.h"
#include "storage/sqlite_operation_repository.h"
#include "storage/storage.h"

namespace mymoney::storage {
namespace {

bool TableExists(sqlite::Database& db, const std::string& name) {
  sqlite::Statement select =
      db.Prepare("SELECT COUNT(*) FROM sqlite_schema WHERE name = ?1", name)
          .value();
  EXPECT_TRUE(select.Step().value());
  return select.Column<std::int64_t>(0) == 1;
}

constexpr std::array<MigrationScript, 2> kTwoScripts{
    MigrationScript{.version = 1,
                    .name = "0001_first.sql",
                    .sql = "CREATE TABLE first (x INTEGER);"},
    MigrationScript{.version = 2,
                    .name = "0002_second.sql",
                    .sql = "CREATE TABLE second (y INTEGER);"},
};

class MigrationsTest : public ::testing::Test {
 protected:
  sqlite::Database db_ = sqlite::Database::Open(":memory:").value();
};

TEST(EmbeddedMigrationsTest, NewDatabaseGetsTheNewestSchema) {
  sqlite::Database db = OpenStorage(":memory:").value();

  EXPECT_EQ(SchemaVersion(db),
            static_cast<int>(EmbeddedMigrationScripts().size()));
  EXPECT_TRUE(TableExists(db, "movement"));
}

TEST_F(MigrationsTest, AppliesAllScriptsToANewDatabase) {
  ASSERT_TRUE(ApplyMigrations(db_, kTwoScripts));

  EXPECT_EQ(SchemaVersion(db_), 2);
  EXPECT_TRUE(TableExists(db_, "first"));
  EXPECT_TRUE(TableExists(db_, "second"));
}

TEST_F(MigrationsTest, AppliesOnlyMissingScripts) {
  ASSERT_TRUE(ApplyMigrations(db_, std::span(kTwoScripts).first(1)));
  EXPECT_EQ(SchemaVersion(db_), 1);

  // Script 1 would fail if it ran again ("table first already exists").
  ASSERT_TRUE(ApplyMigrations(db_, kTwoScripts));
  EXPECT_EQ(SchemaVersion(db_), 2);
}

TEST_F(MigrationsTest, RunningAgainChangesNothing) {
  ASSERT_TRUE(ApplyMigrations(db_, kTwoScripts));
  ASSERT_TRUE(ApplyMigrations(db_, kTwoScripts));

  EXPECT_EQ(SchemaVersion(db_), 2);
}

TEST_F(MigrationsTest, RefusesADatabaseFromANewerApp) {
  ASSERT_TRUE(db_.Execute("PRAGMA user_version = 3"));

  const core::Status status = ApplyMigrations(db_, kTwoScripts);

  ASSERT_FALSE(status.has_value());
  EXPECT_EQ(status.error().code, core::ErrorCode::kStorage);
}

TEST_F(MigrationsTest, FailingScriptLeavesNoTrace) {
  const std::array<MigrationScript, 2> scripts{
      kTwoScripts.front(),
      MigrationScript{.version = 2,
                      .name = "0002_broken.sql",
                      .sql = "CREATE TABLE second (y INTEGER); NOT SQL;"},
  };

  ASSERT_FALSE(ApplyMigrations(db_, scripts));

  // Script 1 is applied; script 2 is rolled back completely - including its
  // first statement and the version update.
  EXPECT_EQ(SchemaVersion(db_), 1);
  EXPECT_TRUE(TableExists(db_, "first"));
  EXPECT_FALSE(TableExists(db_, "second"));
}

// Migration 0003 (ADR-0015) converts the data of schema 2: a category with
// children becomes a group, and its own operations move to "<name> (other)".
TEST(CategoryGroupsMigrationTest, ConvertsParentsIntoGroups) {
  sqlite::Database db = sqlite::Database::Open(":memory:").value();
  ASSERT_TRUE(ApplyMigrations(db, EmbeddedMigrationScripts().first(2)));
  const core::Status setup = db.Execute(R"sql(
      INSERT INTO currency (code, minor_digits) VALUES ('EUR', 2);
      INSERT INTO account (id, name, currency) VALUES (1, 'Cash', 'EUR');
      INSERT INTO category (id, name, kind, parent_id) VALUES
        (1, 'Food', 'expense', NULL),
        (2, 'Restaurants', 'expense', 1),
        (3, 'Home', 'expense', NULL),
        (4, 'Rent', 'expense', 3),
        (5, 'Transport', 'expense', NULL);
      INSERT INTO operation (id, date, type) VALUES
        (1, '2026-10-18', 'expense'),
        (2, '2026-10-18', 'expense'),
        (3, '2026-10-18', 'expense'),
        (4, '2026-10-18', 'expense');
      INSERT INTO movement (operation_id, account_id, category_id, amount_minor)
      VALUES (1, 1, 1, -100), (2, 1, 2, -1250), (3, 1, 4, -50000),
             (4, 1, 5, -300);
  )sql");
  ASSERT_TRUE(setup.has_value()) << setup.error().message;

  ASSERT_TRUE(ApplyMigrations(db, EmbeddedMigrationScripts()));

  SqliteCategoryRepository categories(&db);
  SqliteOperationRepository operations(&db);
  using core::CategoryId;
  EXPECT_TRUE(categories.Get(CategoryId(1)).value().is_group);   // Food
  EXPECT_FALSE(categories.Get(CategoryId(2)).value().is_group);  // Restaurants
  EXPECT_TRUE(categories.Get(CategoryId(3)).value().is_group);   // Home
  EXPECT_FALSE(categories.Get(CategoryId(5)).value().is_group);  // Transport
  // Home had no operations of its own: no "Home (other)".
  EXPECT_EQ(categories.GetByName("Home (other)").error().code,
            core::ErrorCode::kNotFound);

  const core::Result<core::Category> other =
      categories.GetByName("Food (other)");
  ASSERT_TRUE(other.has_value()) << other.error().message;
  EXPECT_EQ(other->kind, core::CategoryKind::kExpense);
  EXPECT_EQ(other->parent_id, CategoryId(1));
  EXPECT_FALSE(other->is_group);
  const core::Result<core::Operation> moved =
      operations.Get(core::OperationId(1));
  ASSERT_TRUE(moved.has_value()) << moved.error().message;
  EXPECT_EQ(moved->draft.movements().front().category_id, other->id);

  // No money was lost on the way.
  EXPECT_EQ(operations.SumMovements(core::AccountId(1), std::nullopt),
            -100 - 1250 - 50000 - 300);
}

}  // namespace
}  // namespace mymoney::storage
