# mymoney_core: domain logic in pure C++23, no Qt (ADR-0002, ADR-0012).

configure_file(version.cpp.in version.cpp @ONLY)

add_library(mymoney_core STATIC
  account.h
  amount_text.cpp
  amount_text.h
  category.h
  check.cpp
  check.h
  currency.cpp
  currency.h
  currency_code.cpp
  currency_code.h
  date_text.cpp
  date_text.h
  error.h
  money.h
  operation.cpp
  operation.h
  strong_id.h
  validation.cpp
  validation.h
  version.h
  "${CMAKE_CURRENT_BINARY_DIR}/version.cpp"

  repositories/account_repository.h
  repositories/category_repository.h
  repositories/currency_repository.h
  repositories/operation_repository.h

  services/account_service.cpp
  services/account_service.h
  services/balance_service.cpp
  services/balance_service.h
  services/category_service.cpp
  services/category_service.h
  services/operation_service.cpp
  services/operation_service.h
)
add_library(mymoney::core ALIAS mymoney_core)

# Includes are written from src/: #include "core/money.h".
target_include_directories(mymoney_core PUBLIC "${PROJECT_SOURCE_DIR}/src")

mymoney_target_defaults(mymoney_core)
