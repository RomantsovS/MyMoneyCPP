#include "storage/sqlite_category_repository.h"

#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/category.h"
#include "core/check.h"
#include "core/error.h"
#include "core/strong_id.h"
#include "storage/sqlite/database.h"
#include "storage/sqlite/statement.h"
#include "storage/text_mapping.h"

namespace mymoney::storage {
namespace {

constexpr std::string_view kSelectCategory =
    "SELECT id, name, kind, parent_id FROM category";

// Reads a row (id, name, kind, parent_id).
core::Result<core::Category> ReadCategory(const sqlite::Statement& row) {
  return CategoryKindFromText(row.Column<std::string>(2))
      .transform([&](core::CategoryKind kind) {
        const auto parent_id = row.Column<std::optional<std::int64_t>>(3);
        return core::Category{
            .id = core::CategoryId(row.Column<std::int64_t>(0)),
            .name = row.Column<std::string>(1),
            .kind = kind,
            .parent_id = parent_id.transform(
                [](std::int64_t value) { return core::CategoryId(value); })};
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
      ->Run("INSERT INTO category (name, kind, parent_id) VALUES (?1, ?2, ?3)",
            category.name, CategoryKindToText(category.kind), parent_id)
      .transform([&] {
        return core::Category{.id = core::CategoryId(db_->LastInsertId()),
                              .name = category.name,
                              .kind = category.kind,
                              .parent_id = category.parent_id};
      });
}

core::Result<core::Category> SqliteCategoryRepository::Get(
    core::CategoryId id) {
  core::Result<sqlite::Statement> select = db_->Prepare(
      std::format("{} WHERE id = ?1", kSelectCategory), id.value());
  if (!select) {
    return std::unexpected(std::move(select).error());
  }
  return select->Step().and_then(
      [&](bool has_row) -> core::Result<core::Category> {
        if (!has_row) {
          return core::MakeError(
              core::ErrorCode::kNotFound,
              std::format("no category with id {}", id.value()));
        }
        return ReadCategory(*select);
      });
}

core::Result<std::vector<core::Category>> SqliteCategoryRepository::List() {
  std::vector<core::Category> categories;
  return db_->Prepare(std::format("{} ORDER BY id", kSelectCategory))
      .and_then([&](sqlite::Statement select) {
        return select.ForEachRow([&](const sqlite::Statement& row) {
          return ReadCategory(row).transform([&](core::Category category) {
            categories.push_back(std::move(category));
          });
        });
      })
      .transform([&] { return std::move(categories); });
}

}  // namespace mymoney::storage
