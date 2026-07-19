add_executable(common_tests
    common/OrderTests.cpp
    common/TradeTests.cpp
    common/PriceLevelTests.cpp
    common/OrderBookTests.cpp
    common/MatchingEngineTests.cpp
)

target_link_libraries(common_tests
    PRIVATE
        common
        GTest::gtest_main
)

exchange_set_warnings(common_tests)

include(GoogleTest)
gtest_discover_tests(common_tests)