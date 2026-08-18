#include <gtest/gtest.h>

#include "gateway/MessageDispatcher.hpp"
#include "gateway/RemoteOrderProcessor.hpp"
#include "matching_engine/MatchingDispatcher.hpp"
#include "risk/RiskDispatcher.hpp"
#include "TestFrameServer.hpp"

#include <chrono>
#include <thread>

using namespace exchange;
using namespace exchange::common::protocol;

TEST(EndToEndIntegrationTest, GatewayRiskMatchingEngineFullChainProducesTrade) {
    constexpr unsigned short mePort = 19200;
    constexpr unsigned short riskPort = 19300;

    matching_engine::MatchingDispatcher meDispatcher;
    test_support::TestFrameServer meServer(
        mePort, [&meDispatcher](const DecodedFrame& f) { return meDispatcher.handle(f); },
        "test_me");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    risk::RiskDispatcher riskDispatcher(risk::RiskConfig{common::Quantity(10'000)},
                                        risk::Account("acct", common::BuyingPower(1'000'000)),
                                        "127.0.0.1", mePort);
    test_support::TestFrameServer riskServer(
        riskPort, [&riskDispatcher](const DecodedFrame& f) { return riskDispatcher.handle(f); },
        "test_risk");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    gateway::RemoteOrderProcessor remoteProcessor("127.0.0.1", riskPort);
    gateway::MessageDispatcher gatewayDispatcher(remoteProcessor);

    common::Order sell(common::OrderId(1), "AAPL", common::Side::Sell, common::Price(100),
                       common::Quantity(10), common::Timestamp{});
    ByteWriter w1;
    writeOrder(w1, sell);
    gatewayDispatcher.handle(*tryDecodeFrame(encodeFrame(MessageType::NewOrder, w1.bytes())));

    common::Order buy(common::OrderId(2), "AAPL", common::Side::Buy, common::Price(100),
                      common::Quantity(10), common::Timestamp{});
    ByteWriter w2;
    writeOrder(w2, buy);
    auto responseBytes =
        gatewayDispatcher.handle(*tryDecodeFrame(encodeFrame(MessageType::NewOrder, w2.bytes())));
    auto response = tryDecodeFrame(responseBytes);

    ASSERT_TRUE(response.has_value());
    EXPECT_EQ(response->type, MessageType::OrderAck);
    ByteReader reader(response->payload);
    auto ack = readOrderAck(reader);
    ASSERT_EQ(ack.trades.size(), 1u);
    EXPECT_EQ(ack.trades[0].price(), common::Price(100));
}