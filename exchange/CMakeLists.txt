cmake_minimum_required(VERSION 3.25)

project(mini_exchange
    VERSION 0.1.0
    DESCRIPTION "Mini Exchange Simulator - low-latency architecture demo"
    LANGUAGES CXX
)

# --- Global settings -------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # portable, no compiler-specific dialects

# Always emit compile_commands.json - required by clang-tidy and useful for
# VS Code / CLion tooling too.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep binaries in predictable places regardless of subdirectory nesting.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(${CMAKE_SOURCE_DIR}/cmake/CompilerWarnings.cmake)

# --- Options -----------------------------------------------------------

option(EXCHANGE_ENABLE_TESTS "Build unit tests" ON)
option(EXCHANGE_ENABLE_CLANG_TIDY "Run clang-tidy during build" OFF)

if(EXCHANGE_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# --- Subprojects -------------------------------------------------------
# Order matters only for readability here; CMake resolves dependencies
# via target_link_libraries regardless of add_subdirectory order.

add_subdirectory(common)

# Services are added in later milestones as they're implemented:
# add_subdirectory(gateway)
# add_subdirectory(matching_engine)
# add_subdirectory(risk)
# add_subdirectory(market_data)
# add_subdirectory(persistence)
# add_subdirectory(client)

if(EXCHANGE_ENABLE_TESTS)
    enable_testing()
    # add_subdirectory(tests)   # added once GoogleTest is wired in
endif()