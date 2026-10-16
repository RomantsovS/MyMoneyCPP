#include "core/version.h"

#include <regex>
#include <string>

#include <gtest/gtest.h>

namespace mymoney::core {
namespace {

TEST(VersionTest, IsMajorMinorPatch) {
  const std::string version(Version());

  EXPECT_TRUE(std::regex_match(version, std::regex(R"(\d+\.\d+\.\d+)")))
      << "version: " << version;
}

}  // namespace
}  // namespace mymoney::core
