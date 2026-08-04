add_library(common STATIC
    src/Logger.cpp
)

target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(common PUBLIC cxx_std_20)

target_link_libraries(common
    PUBLIC
        Boost::system
)

exchange_set_warnings(common)


add_executable(asio_smoke_test
    include/common/examples/AsioSmokeTest.cpp
)

target_link_libraries(asio_smoke_test
    PRIVATE
        common
)

exchange_set_warnings(asio_smoke_test)