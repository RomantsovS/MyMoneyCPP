#include "storage/sqlite_category_repository.h"

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/strong_id.h"
#include "storage/query_helpers.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"
#include "storage/text_mapping.h"

namespace mymoney::storage {
namespace {

constexpr std::string_view kSelectCategory =
    "SELECT id, name, kind, parent_id, is_group FROM category";

// Reads a row (id, name, kind, parent_id, is_group).
core::Result<core::Category> ReadCategory(const sqlite::Statement& row) {
  return CategoryKindFromText(row.Column<std::string>(2))
      .transform([&](core::CategoryKind kind) {
        const auto parent_id = row.Column<std::optional<std::int64_t>>(3);
        return core::Category{
            .id = core::CategoryId(row.Column<std::int64_t>(0)),
            .name = row.Column<std::string>(1),
            .kind = kind,
            .parent_id = parent_id.transform(
                [](std::int64_t value) { return core::CategoryId(value); }),
            .is_group = row.Column<bool>(4)};
      });
}

}  // namespace

SqliteCategoryRepository::SqliteCategoryRepository(sqlite::Database* db)
    : db_(db) {
  core::Check(db_ != nullptr, "SqliteCategoryRepository needs a Database");
}

core::Result<core::Category> SqliteCategoryRepository::Add(
    const core::NewCategory& category) {
  const std::optional<std::int64_t> parent_id = category.parent_id.transform(
      [](core::CategoryId id) { return id.value(); });
  return db_
      ->Run(
          "INSERT INTO category (name, kind, parent_id, is_group) "
          "VALUES (?1, ?2, ?3, ?4)",
          category.name, CategoryKindToText(category.kind), parent_id,
          category.is_group)
      .transform([&] {
        return core::Category{.id = core::CategoryId(db_->LastInsertId()),
                              .name = category.name,
                              .kind = category.kind,
                              .parent_id = category.parent_id,
                              .is_group = category.is_group};
      });
}

core::Result<core::Category> SqliteCategoryRepository::Get(
    core::CategoryId id) {
  return internal::QueryOne(*db_,
                            std::format("{} WHERE id = ?1", kSelectCategory),
                            std::format("no category with id {}", id.value()),
                            ReadCategory, id.value());
}

core::Result<core::Category> SqliteCategoryRepository::GetByName(
    std::string_view name) {
  return internal::QueryOne(
      *db_, std::format("{} WHERE name = ?1", kSelectCategory),
      std::format("no category named '{}'", name), ReadCategory, name);
}

core::Result<std::vector<core::Category>> SqliteCategoryRepository::List() {
  return internal::QueryAll<core::Category>(
      *db_, std::format("{} ORDER BY id", kSelectCategory), ReadCategory);
}

core::Status SqliteCategoryRepository::Rename(core::CategoryId id,
                                              std::string_view name) {
  return db_
      ->Run("UPDATE category SET name = ?2 WHERE id = ?1", id.value(), name)
      .and_then([&]() -> core::Status {
        if (db_->ChangedRows() == 0) {
          return core::MakeError(
              core::ErrorCode::kNotFound,
              std::format("no category with id {}", id.value()));
        }
        return {};
      });
}

}  // namespace mymoney::storage
