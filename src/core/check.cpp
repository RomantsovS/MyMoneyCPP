#include "core/check.h"

#include <cstdio>
#include <cstdlib>
#include <print>
#include <source_location>
#include <string_view>

namespace mymoney::core {

void CheckFailed(std::string_view what, const std::source_location& location) {
  std::println(stderr, "{}:{}: check failed: {} (in {})", location.file_name(),
               location.line(), what, location.function_name());
  std::abort();
}

}  // namespace mymoney::core
