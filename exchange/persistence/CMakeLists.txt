add_library(persistence_lib INTERFACE)

target_include_directories(
    persistence_lib
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    find_package(libpqxx CONFIG REQUIRED)
    find_package(RdKafka CONFIG)

    if(NOT TARGET RdKafka::rdkafka++)
        message(FATAL_ERROR
            "RdKafka::rdkafka++ was not found. "
            "Check that librdkafka:x64-windows is installed."
        )
    endif()

    target_link_libraries(
        persistence_lib
        INTERFACE
            common
            libpqxx::pqxx
            RdKafka::rdkafka++
    )
else()
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(
        PQXX
        REQUIRED
        IMPORTED_TARGET
        libpqxx
    )

    pkg_check_modules(
        RdKafka
        REQUIRED
        IMPORTED_TARGET
        rdkafka++
    )

    target_link_libraries(
        persistence_lib
        INTERFACE
            common
            PkgConfig::PQXX
            PkgConfig::RdKafka
    )
endif()

target_compile_features(
    persistence_lib
    INTERFACE
        cxx_std_20
)

add_executable(
    persistence_service
    src/main.cpp
)

target_link_libraries(
    persistence_service
    PRIVATE
        persistence_lib
)

exchange_set_warnings(persistence_service)