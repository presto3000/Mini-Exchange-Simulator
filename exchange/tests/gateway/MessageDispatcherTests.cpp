#include <gtest/gtest.h>

#include "gateway/InProcessOrderProcessor.hpp"
#include "gateway/MessageDispatcher.hpp"

using namespace exchange;
using namespace exchange::common;
using namespace exchange::common::protocol;
using namespace exchange::gateway;

namespace {

std::vector<std::byte> buildNewOrderFrame(const Order& order) {
    ByteWriter w;
    writeOrder(w, order);
    return encodeFrame(MessageType::NewOrder, w.bytes());
}

} // namespace

TEST(MessageDispatcherTest, ValidNewOrderProducesAckFrame) {
    InProcessOrderProcessor processor(risk::RiskConfig{Quantity(10'000)},
                                      risk::Account("a", BuyingPower(1'000'000)));
    MessageDispatcher dispatcher(processor);

    Order order(OrderId(1), "AAPL", Side::Buy, Price(100), Quantity(10), Timestamp{});
    auto frameBytes = buildNewOrderFrame(order);
    auto decoded = tryDecodeFrame(frameBytes);
    ASSERT_TRUE(decoded.has_value());

    auto responseBytes = dispatcher.handle(*decoded);
    auto responseFrame = tryDecodeFrame(responseBytes);
    ASSERT_TRUE(responseFrame.has_value());
    EXPECT_EQ(responseFrame->type, MessageType::OrderAck);

    ByteReader reader(responseFrame->payload);
    auto ack = readOrderAck(reader);
    EXPECT_EQ(ack.orderId, OrderId(1));
}

TEST(MessageDispatcherTest, RiskRejectedOrderProducesRejectFrame) {
    InProcessOrderProcessor processor(risk::RiskConfig{Quantity(10'000)},
                                      risk::Account("a", BuyingPower(1))); // too low
    MessageDispatcher dispatcher(processor);

    Order order(OrderId(1), "AAPL", Side::Buy, Price(100), Quantity(10), Timestamp{});
    auto decoded = tryDecodeFrame(buildNewOrderFrame(order));
    ASSERT_TRUE(decoded.has_value());

    auto responseBytes = dispatcher.handle(*decoded);
    auto responseFrame = tryDecodeFrame(responseBytes);
    ASSERT_TRUE(responseFrame.has_value());
    EXPECT_EQ(responseFrame->type, MessageType::OrderReject);
}

TEST(MessageDispatcherTest, TruncatedPayloadProducesFormatRejectNotCrash) {
    InProcessOrderProcessor processor(risk::RiskConfig{Quantity(10'000)},
                                      risk::Account("a", BuyingPower(1'000'000)));
    MessageDispatcher dispatcher(processor);

    // A NewOrder frame whose payload is far too short to contain a full
    // Order - simulates a malformed/corrupt message, exactly the
    // scenario ByteReader's std::out_of_range guards against.
    std::vector<std::byte> tinyPayload{std::byte{1}, std::byte{2}};
    DecodedFrame fakeFrame{MessageType::NewOrder, tinyPayload, 0};

    auto responseBytes = dispatcher.handle(fakeFrame);
    auto responseFrame = tryDecodeFrame(responseBytes);
    ASSERT_TRUE(responseFrame.has_value());
    EXPECT_EQ(responseFrame->type, MessageType::OrderReject); // rejected, not a crash
}

TEST(MessageDispatcherTest, UnknownMessageTypeIsRejected) {
    InProcessOrderProcessor processor(risk::RiskConfig{Quantity(10'000)},
                                      risk::Account("a", BuyingPower(1'000'000)));
    MessageDispatcher dispatcher(processor);

    DecodedFrame unknownFrame{static_cast<MessageType>(255), {}, 0};
    auto responseBytes = dispatcher.handle(unknownFrame);
    auto responseFrame = tryDecodeFrame(responseBytes);
    ASSERT_TRUE(responseFrame.has_value());
    EXPECT_EQ(responseFrame->type, MessageType::OrderReject);
}

TEST(MessageDispatcherTest, CancelMessageRoundTrips) {
    InProcessOrderProcessor processor(risk::RiskConfig{Quantity(10'000)},
                                      risk::Account("a", BuyingPower(1'000'000)));
    MessageDispatcher dispatcher(processor);

    Order order(OrderId(1), "AAPL", Side::Buy, Price(100), Quantity(10), Timestamp{});
    dispatcher.handle(*tryDecodeFrame(buildNewOrderFrame(order)));

    ByteWriter w;
    writeCancelOrder(w, {OrderId(1)});
    auto cancelFrame = encodeFrame(MessageType::CancelOrder, w.bytes());

    auto responseBytes = dispatcher.handle(*tryDecodeFrame(cancelFrame));
    auto responseFrame = tryDecodeFrame(responseBytes);
    ASSERT_TRUE(responseFrame.has_value());
    EXPECT_EQ(responseFrame->type, MessageType::OrderAck);
}