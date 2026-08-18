#include <gtest/gtest.h>

#include "matching_engine/MatchingDispatcher.hpp"
#include "risk/RiskDispatcher.hpp"
#include "TestFrameServer.hpp"

#include <chrono>
#include <thread>

using namespace exchange;
using namespace exchange::common::protocol;

TEST(RiskDispatcherIntegrationTest, AcceptedOrderForwardsAndReturnsTradeFromRealMatchingEngine) {
    constexpr unsigned short mePort = 19100;

    matching_engine::MatchingDispatcher meDispatcher;
    test_support::TestFrameServer meServer(
        mePort, [&meDispatcher](const DecodedFrame& f) { return meDispatcher.handle(f); },
        "test_matching_engine");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let the acceptor start

    risk::RiskDispatcher dispatcher(risk::RiskConfig{common::Quantity(10'000)},
                                    risk::Account("acct", common::BuyingPower(1'000'000)),
                                    "127.0.0.1", mePort);

    // Rest a sell order first.
    common::Order sellOrder(common::OrderId(1), "AAPL", common::Side::Sell, common::Price(100),
                            common::Quantity(10), common::Timestamp{});
    ByteWriter w1;
    writeOrder(w1, sellOrder);
    auto sellFrame = encodeFrame(MessageType::NewOrder, w1.bytes());
    ASSERT_TRUE(tryDecodeFrame(dispatcher.handle(*tryDecodeFrame(sellFrame))).has_value());

    // Submit a crossing buy order through RiskDispatcher.
    common::Order buyOrder(common::OrderId(2), "AAPL", common::Side::Buy, common::Price(100),
                           common::Quantity(10), common::Timestamp{});
    ByteWriter w2;
    writeOrder(w2, buyOrder);
    auto buyResponseBytes =
        dispatcher.handle(*tryDecodeFrame(encodeFrame(MessageType::NewOrder, w2.bytes())));
    auto buyResponse = tryDecodeFrame(buyResponseBytes);

    ASSERT_TRUE(buyResponse.has_value());
    EXPECT_EQ(buyResponse->type, MessageType::OrderAck);
    ByteReader reader(buyResponse->payload);
    auto ack = readOrderAck(reader);
    ASSERT_EQ(ack.trades.size(), 1u);
    EXPECT_EQ(ack.trades[0].price(), common::Price(100));
}

TEST(RiskDispatcherIntegrationTest, RiskRejectedOrderNeverReachesMatchingEngine) {
    constexpr unsigned short mePort = 19101;

    matching_engine::MatchingDispatcher meDispatcher;
    test_support::TestFrameServer meServer(
        mePort, [&meDispatcher](const DecodedFrame& f) { return meDispatcher.handle(f); },
        "test_matching_engine");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    risk::RiskDispatcher dispatcher(risk::RiskConfig{common::Quantity(10'000)},
                                    risk::Account("acct", common::BuyingPower(1)), // too low
                                    "127.0.0.1", mePort);

    common::Order order(common::OrderId(1), "AAPL", common::Side::Buy, common::Price(100),
                        common::Quantity(10), common::Timestamp{});
    ByteWriter w;
    writeOrder(w, order);
    auto responseBytes =
        dispatcher.handle(*tryDecodeFrame(encodeFrame(MessageType::NewOrder, w.bytes())));
    auto response = tryDecodeFrame(responseBytes);

    ASSERT_TRUE(response.has_value());
    EXPECT_EQ(response->type, MessageType::OrderReject); // stopped at Risk
}