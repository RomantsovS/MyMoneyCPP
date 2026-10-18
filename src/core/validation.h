#ifndef MYMONEY_CORE_VALIDATION_H_
#define MYMONEY_CORE_VALIDATION_H_

#include <string_view>

#include "core/error.h"

namespace mymoney::core {

// Checks a user-entered name: it must contain more than whitespace.
// `what` names the object in the error message, e.g. "account".
[[nodiscard]] Status ValidateName(std::string_view name, std::string_view what);

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_VALIDATION_H_
