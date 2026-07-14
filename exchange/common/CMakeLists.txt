add_library(common STATIC
    src/Logger.cpp
 "include/common/PriceLevel.hpp")

target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(common PUBLIC cxx_std_20)

exchange_set_warnings(common)