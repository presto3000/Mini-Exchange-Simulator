#include <gtest/gtest.h>

#include "gateway/InProcessOrderProcessor.hpp"

using namespace exchange;
using namespace exchange::common;
using namespace exchange::gateway;

namespace {
InProcessOrderProcessor makeProcessor(std::int64_t buyingPower = 1'000'000) {
    return InProcessOrderProcessor(risk::RiskConfig{Quantity(10'000)},
                                   risk::Account("acct-1", BuyingPower(buyingPower)));
}

Order makeOrder(OrderId id, Side side, Price price, Quantity qty) {
    return Order(id, "AAPL", side, price, qty, Timestamp{});
}
} // namespace

TEST(InProcessOrderProcessorTest, AcceptedOrderRestsWithZeroTrades) {
    auto processor = makeProcessor();
    auto result = processor.submitOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    ASSERT_TRUE(std::holds_alternative<protocol::OrderAckMessage>(result));
    EXPECT_TRUE(std::get<protocol::OrderAckMessage>(result).trades.empty());
}

TEST(InProcessOrderProcessorTest, RiskRejectionReturnsRejectMessage) {
    auto processor = makeProcessor(/*buyingPower=*/10); // too low
    auto result = processor.submitOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    ASSERT_TRUE(std::holds_alternative<protocol::OrderRejectMessage>(result));
}

TEST(InProcessOrderProcessorTest, CrossingOrdersProduceTradeInAck) {
    auto processor = makeProcessor();
    processor.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(10)));

    auto result = processor.submitOrder(makeOrder(OrderId(2), Side::Buy, Price(100), Quantity(10)));

    ASSERT_TRUE(std::holds_alternative<protocol::OrderAckMessage>(result));
    EXPECT_EQ(std::get<protocol::OrderAckMessage>(result).trades.size(), 1u);
}

TEST(InProcessOrderProcessorTest, DifferentSymbolsUseIndependentEngines) {
    auto processor = makeProcessor();
    processor.submitOrder(
        Order(OrderId(1), "AAPL", Side::Sell, Price(100), Quantity(10), Timestamp{}));
    auto result = processor.submitOrder(
        Order(OrderId(2), "MSFT", Side::Buy, Price(100), Quantity(10), Timestamp{}));

    // MSFT buy should NOT match against AAPL's resting sell.
    ASSERT_TRUE(std::holds_alternative<protocol::OrderAckMessage>(result));
    EXPECT_TRUE(std::get<protocol::OrderAckMessage>(result).trades.empty());
}

TEST(InProcessOrderProcessorTest, CancelExistingOrderSucceeds) {
    auto processor = makeProcessor();
    processor.submitOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    auto result = processor.cancelOrder(OrderId(1));
    EXPECT_TRUE(std::holds_alternative<protocol::OrderAckMessage>(result));
}

TEST(InProcessOrderProcessorTest, CancelUnknownOrderRejects) {
    auto processor = makeProcessor();
    auto result = processor.cancelOrder(OrderId(999));
    EXPECT_TRUE(std::holds_alternative<protocol::OrderRejectMessage>(result));
}