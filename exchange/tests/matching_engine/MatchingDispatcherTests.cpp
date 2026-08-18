#include <gtest/gtest.h>

#include "matching_engine/MatchingDispatcher.hpp"

using namespace exchange;
using namespace exchange::common::protocol;

namespace {
std::vector<std::byte> buildNewOrderFrame(const common::Order& order) {
    ByteWriter w;
    writeOrder(w, order);
    return encodeFrame(MessageType::NewOrder, w.bytes());
}
} // namespace

TEST(MatchingDispatcherTest, CrossingOrdersProduceTradeInAck) {
    matching_engine::MatchingDispatcher dispatcher;

    common::Order sell(common::OrderId(1), "AAPL", common::Side::Sell, common::Price(100),
                       common::Quantity(10), common::Timestamp{});
    dispatcher.handle(*tryDecodeFrame(buildNewOrderFrame(sell)));

    common::Order buy(common::OrderId(2), "AAPL", common::Side::Buy, common::Price(100),
                      common::Quantity(10), common::Timestamp{});
    auto responseBytes = dispatcher.handle(*tryDecodeFrame(buildNewOrderFrame(buy)));
    auto response = tryDecodeFrame(responseBytes);

    ASSERT_TRUE(response.has_value());
    EXPECT_EQ(response->type, MessageType::OrderAck);
    ByteReader reader(response->payload);
    EXPECT_EQ(readOrderAck(reader).trades.size(), 1u);
}

TEST(MatchingDispatcherTest, CancelUnknownOrderRejects) {
    matching_engine::MatchingDispatcher dispatcher;
    ByteWriter w;
    writeCancelOrder(w, {common::OrderId(999)});
    auto responseBytes =
        dispatcher.handle(*tryDecodeFrame(encodeFrame(MessageType::CancelOrder, w.bytes())));
    auto response = tryDecodeFrame(responseBytes);
    ASSERT_TRUE(response.has_value());
    EXPECT_EQ(response->type, MessageType::OrderReject);
}

TEST(MatchingDispatcherTest, MalformedPayloadRejectsCleanly) {
    matching_engine::MatchingDispatcher dispatcher;
    DecodedFrame fakeFrame{MessageType::NewOrder, {std::byte{1}, std::byte{2}}, 0};
    auto responseBytes = dispatcher.handle(fakeFrame);
    auto response = tryDecodeFrame(responseBytes);
    ASSERT_TRUE(response.has_value());
    EXPECT_EQ(response->type, MessageType::OrderReject);
}