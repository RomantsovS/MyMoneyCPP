# Compiler settings shared by all MyMoney targets (ADR-0011).
#
# They live on an INTERFACE target instead of global CMAKE_CXX_FLAGS, so they
# apply only to targets that link it - never to third-party code such as
# GoogleTest, where warnings-as-errors would break the build on updates.

add_library(mymoney_compile_options INTERFACE)
add_library(mymoney::compile_options ALIAS mymoney_compile_options)

target_compile_features(mymoney_compile_options INTERFACE cxx_std_23)

if(MSVC)
  target_compile_options(mymoney_compile_options INTERFACE
    /W4              # high warning level
    /permissive-     # standard conformance
    /utf-8           # source and execution character sets are UTF-8
    /Zc:__cplusplus  # report the real C++ version in __cplusplus
    /Zc:preprocessor # standard-conforming preprocessor
  )
else()
  target_compile_options(mymoney_compile_options INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion        # implicit narrowing, e.g. int64_t -> int (money!)
    -Wsign-conversion
    -Wold-style-cast    # Google style: no C-style casts
    -Wnon-virtual-dtor
  )
endif()

# Applies the project defaults to one of our own targets.
function(mymoney_target_defaults target)
  target_link_libraries(${target} PRIVATE mymoney::compile_options)
  set_target_properties(${target} PROPERTIES
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON)
endfunction()
