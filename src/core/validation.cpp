#include "core/validation.h"

#include <algorithm>
#include <format>
#include <string_view>

#include "core/error.h"

namespace mymoney::core {

Status ValidateName(std::string_view name, std::string_view what) {
  const bool blank = std::ranges::all_of(name, [](char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  });
  if (blank) {
    return MakeError(ErrorCode::kInvalidArgument,
                     std::format("{} name must not be empty", what));
  }
  return {};
}

}  // namespace mymoney::core
