add_library(matching_engine_lib INTERFACE)

target_include_directories(
    matching_engine_lib
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    find_package(RdKafka CONFIG)

    if(NOT TARGET RdKafka::rdkafka++)
        message(FATAL_ERROR
            "RdKafka::rdkafka++ was not found. "
            "Check that librdkafka:x64-windows is installed."
        )
    endif()

    target_link_libraries(
        matching_engine_lib
        INTERFACE
            RdKafka::rdkafka++
    )
else()
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(
        RdKafka
        REQUIRED
        IMPORTED_TARGET
        rdkafka++
    )

    target_link_libraries(
        matching_engine_lib
        INTERFACE
            PkgConfig::RdKafka
    )
endif()

target_link_libraries(
    matching_engine_lib
    INTERFACE
        common
        Boost::system
        cppzmq_headers
)

target_compile_features(
    matching_engine_lib
    INTERFACE
        cxx_std_20
)

add_executable(
    matching_engine_service
    src/main.cpp
)

target_link_libraries(
    matching_engine_service
    PRIVATE
        matching_engine_lib
)

exchange_set_warnings(matching_engine_service)