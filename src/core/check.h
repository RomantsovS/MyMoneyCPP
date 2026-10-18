#ifndef MYMONEY_CORE_CHECK_H_
#define MYMONEY_CORE_CHECK_H_

#include <source_location>
#include <string_view>

namespace mymoney::core {

// Prints the failed check with its source location to stderr and aborts.
[[noreturn]] void CheckFailed(std::string_view what,
                              const std::source_location& location);

// Verifies a precondition or invariant - a programming error if it fails
// (ADR-0012). Unlike assert, it stays active in Release builds: a broken
// invariant in money code must stop the program, not continue silently.
// Expected failures (bad input, missing data) are returned as Error values.
constexpr void Check(
    bool condition, std::string_view what,
    std::source_location location = std::source_location::current()) {
  if (!condition) [[unlikely]] {
    CheckFailed(what, location);
  }
}

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_CHECK_H_
