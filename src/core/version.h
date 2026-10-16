#ifndef MYMONEY_CORE_VERSION_H_
#define MYMONEY_CORE_VERSION_H_

#include <string_view>

namespace mymoney::core {

// Returns the application version, e.g. "0.1.0".
//
// The value comes from project(MyMoney VERSION ...) in the top-level
// CMakeLists.txt. It is defined in a generated .cpp file, so changing the
// version recompiles one file instead of everything that includes this header.
std::string_view Version();

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_VERSION_H_
