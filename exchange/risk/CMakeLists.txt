add_library(risk_lib INTERFACE)

target_include_directories(risk_lib
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(risk_lib
    INTERFACE
        common
)

target_compile_features(risk_lib INTERFACE cxx_std_20)

add_executable(risk_service src/main.cpp)
target_link_libraries(risk_service PRIVATE risk_lib)
exchange_set_warnings(risk_service)