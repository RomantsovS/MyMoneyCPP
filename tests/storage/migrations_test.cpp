#include "storage/migrations.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>

#include <gtest/gtest.h>

#include "core/error.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"
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

}  // namespace
}  // namespace mymoney::storage
