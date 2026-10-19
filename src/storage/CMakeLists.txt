# mymoney_storage: SQLite storage behind the core repository interfaces,
# pure C++23 (ADR-0007, ADR-0013).

include(EmbedMigrations)

# Each new schema version is one more file here: 0002_..., 0003_...
set(mymoney_migration_scripts
  migrations/0001_initial.sql
  migrations/0002_unique_names.sql
  migrations/0003_category_groups.sql
)
mymoney_embed_migrations(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/migration_scripts.cpp"
  SCRIPTS ${mymoney_migration_scripts})

add_library(mymoney_storage STATIC
  migrations.cpp
  migrations.h
  query_helpers.h
  sqlite_account_repository.cpp
  sqlite_account_repository.h
  sqlite_category_repository.cpp
  sqlite_category_repository.h
  sqlite_currency_repository.cpp
  sqlite_currency_repository.h
  sqlite_operation_repository.cpp
  sqlite_operation_repository.h
  storage.cpp
  storage.h
  text_mapping.cpp
  text_mapping.h
  "${CMAKE_CURRENT_BINARY_DIR}/migration_scripts.cpp"
  ${mymoney_migration_scripts}  # listed so IDEs show them

  sqlite/database.cpp
  sqlite/database.h
  sqlite/sqlite_error.cpp
  sqlite/sqlite_error.h
  sqlite/statement.cpp
  sqlite/statement.h
  sqlite/transaction.cpp
  sqlite/transaction.h
)
add_library(mymoney::storage ALIAS mymoney_storage)

# Includes are written from src/: #include "storage/storage.h".
target_include_directories(mymoney_storage PUBLIC "${PROJECT_SOURCE_DIR}/src")
# SQLite is PRIVATE: no header of ours exposes sqlite3.h.
target_link_libraries(mymoney_storage
  PUBLIC mymoney::core
  PRIVATE SQLite::SQLite3)

mymoney_target_defaults(mymoney_storage)
