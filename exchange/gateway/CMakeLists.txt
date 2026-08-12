add_library(gateway_lib INTERFACE)

target_include_directories(gateway_lib
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gateway_lib
    INTERFACE
        common
        risk_lib
        Boost::system
)

target_compile_features(gateway_lib INTERFACE cxx_std_20)

add_executable(gateway_service src/main.cpp)
target_link_libraries(gateway_service PRIVATE gateway_lib)
exchange_set_warnings(gateway_service)