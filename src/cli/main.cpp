// mymoney-cli: command-line interface (ADR-0010).
//
// M0 only supports --version and --help. Real commands and an argument
// parser (D22) come in M3.

#include <cstddef>
#include <cstdio>
#include <print>
#include <ranges>
#include <span>
#include <string_view>

#include "core/version.h"

namespace {

constexpr int kExitOk = 0;
constexpr int kExitUsage = 2;  // conventional exit code for wrong usage

void PrintUsage(std::FILE* stream) {
  std::println(stream, "Usage: mymoney-cli [--version] [--help]");
}

}  // namespace

int main(int argc, char* argv[]) {
  // The options without the program name (argv[0]). views::drop on a span
  // returns a span and is safe even if argc is 0.
  const std::span<char*> options =
      std::span(argv, static_cast<std::size_t>(argc)) | std::views::drop(1);

  if (options.size() == 1) {
    const std::string_view option = options.front();
    if (option == "--version") {
      std::println("mymoney-cli {}", mymoney::core::Version());
      return kExitOk;
    }
    if (option == "--help") {
      PrintUsage(stdout);
      return kExitOk;
    }
  }

  PrintUsage(stderr);
  return kExitUsage;
}
