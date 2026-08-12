add_executable(common_tests
    common/OrderTests.cpp
    common/TradeTests.cpp
    common/PriceLevelTests.cpp
    common/OrderBookTests.cpp
    common/MatchingEngineTests.cpp
)

target_link_libraries(common_tests PRIVATE common GTest::gtest_main)
exchange_set_warnings(common_tests)

add_executable(protocol_tests
    common/protocol/SerializationTests.cpp
    common/protocol/FrameTests.cpp
)

target_link_libraries(protocol_tests PRIVATE common GTest::gtest_main)
exchange_set_warnings(protocol_tests)

add_executable(risk_tests
    risk/RiskCheckerTests.cpp
)

target_link_libraries(risk_tests PRIVATE risk_lib GTest::gtest_main)
exchange_set_warnings(risk_tests)

add_executable(gateway_tests
    gateway/InProcessOrderProcessorTests.cpp
    gateway/MessageDispatcherTests.cpp
)

target_link_libraries(gateway_tests PRIVATE gateway_lib GTest::gtest_main)
exchange_set_warnings(gateway_tests)


include(GoogleTest)
gtest_discover_tests(common_tests)
gtest_discover_tests(risk_tests)
gtest_discover_tests(protocol_tests)
gtest_discover_tests(gateway_tests)
