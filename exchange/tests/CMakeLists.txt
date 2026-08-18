add_executable(common_tests
    common/OrderTests.cpp common/TradeTests.cpp common/PriceLevelTests.cpp
    common/OrderBookTests.cpp common/MatchingEngineTests.cpp
    common/protocol/SerializationTests.cpp common/protocol/FrameTests.cpp
)
target_link_libraries(common_tests PRIVATE common GTest::gtest_main)
exchange_set_warnings(common_tests)

add_executable(risk_tests
    risk/RiskCheckerTests.cpp
    risk/RiskDispatcherIntegrationTests.cpp
)
target_link_libraries(risk_tests PRIVATE risk_lib matching_engine_lib GTest::gtest_main)
exchange_set_warnings(risk_tests)

add_executable(matching_engine_tests
    matching_engine/MatchingDispatcherTests.cpp
)
target_link_libraries(matching_engine_tests PRIVATE matching_engine_lib GTest::gtest_main)
exchange_set_warnings(matching_engine_tests)

add_executable(gateway_tests
    gateway/InProcessOrderProcessorTests.cpp
    gateway/MessageDispatcherTests.cpp
    gateway/EndToEndIntegrationTests.cpp
)
target_link_libraries(gateway_tests PRIVATE gateway_lib risk_lib matching_engine_lib GTest::gtest_main)
exchange_set_warnings(gateway_tests)

target_include_directories(risk_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support)
target_include_directories(gateway_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support)

include(GoogleTest)
gtest_discover_tests(common_tests)
gtest_discover_tests(risk_tests)
gtest_discover_tests(matching_engine_tests)
gtest_discover_tests(gateway_tests)