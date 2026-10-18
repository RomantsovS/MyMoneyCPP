#ifndef MYMONEY_CORE_STRONG_ID_H_
#define MYMONEY_CORE_STRONG_ID_H_

#include <compare>
#include <cstdint>

namespace mymoney::core {

// An integer ID that is its own type (ADR-0012): an AccountId cannot be passed
// where a CategoryId is expected, so swapped arguments do not compile.
// There is no default value - every ID comes from the storage.
template <typename Tag>
class StrongId {
 public:
  using ValueType = std::int64_t;

  constexpr explicit StrongId(ValueType value) noexcept : value_(value) {}

  [[nodiscard]] constexpr ValueType value() const noexcept { return value_; }

  friend constexpr auto operator<=>(const StrongId&, const StrongId&) = default;

 private:
  ValueType value_;
};

struct AccountIdTag {};
struct CategoryIdTag {};
struct OperationIdTag {};

using AccountId = StrongId<AccountIdTag>;
using CategoryId = StrongId<CategoryIdTag>;
using OperationId = StrongId<OperationIdTag>;

}  // namespace mymoney::core

#endif  // MYMONEY_CORE_STRONG_ID_H_
