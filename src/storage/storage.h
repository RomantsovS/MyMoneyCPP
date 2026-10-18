#ifndef MYMONEY_STORAGE_STORAGE_H_
#define MYMONEY_STORAGE_STORAGE_H_

#include <string>

#include "core/error.h"
#include "storage/sqlite/database.h"

namespace mymoney::storage {

// Opens (or creates) the MyMoney database at `path` - ":memory:" for tests -
// and makes it ready to use (ADR-0013): foreign keys on, single-file journal
// mode, schema migrated to the newest version.
[[nodiscard]] core::Result<sqlite::Database> OpenStorage(
    const std::string& path);

}  // namespace mymoney::storage

#endif  // MYMONEY_STORAGE_STORAGE_H_
