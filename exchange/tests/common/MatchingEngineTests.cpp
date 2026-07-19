#include <gtest/gtest.h>

#include "common/MatchingEngine.hpp"

using namespace exchange::common;

namespace {
Order makeOrder(OrderId id, Side side, Price price, Quantity qty) {
    return Order(id, "AAPL", side, price, qty, std::chrono::system_clock::now());
}
} // namespace

TEST(MatchingEngineTest, RestingOrderOnEmptyBookGeneratesNoTrade) {
    MatchingEngine engine("AAPL");
    auto trades = engine.submitOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    EXPECT_TRUE(trades.empty());
    ASSERT_TRUE(engine.book().bestBid().has_value());
    EXPECT_EQ(*engine.book().bestBid(), Price(100));
}

TEST(MatchingEngineTest, CrossingOrdersProduceExactFullFill) {
    MatchingEngine engine("AAPL");
    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(50)));

    auto trades = engine.submitOrder(makeOrder(OrderId(2), Side::Buy, Price(100), Quantity(50)));

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price(), Price(100)); // resting order's price
    EXPECT_EQ(trades[0].quantity(), Quantity(50));
    EXPECT_EQ(trades[0].sellOrderId(), OrderId(1));
    EXPECT_EQ(trades[0].buyOrderId(), OrderId(2));

    EXPECT_TRUE(engine.book().empty()); // both orders fully consumed
}

TEST(MatchingEngineTest, PartialFillLeavesRemainderResting) {
    MatchingEngine engine("AAPL");
    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(30)));

    auto trades = engine.submitOrder(makeOrder(OrderId(2), Side::Buy, Price(100), Quantity(50)));

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity(), Quantity(30)); // only what was available

    // Buyer's remaining 20 should now be resting on the book.
    ASSERT_TRUE(engine.book().bestBid().has_value());
    EXPECT_EQ(*engine.book().bestBid(), Price(100));
    auto resting = engine.book().findOrder(OrderId(2));
    ASSERT_TRUE(resting.has_value());
    EXPECT_EQ(resting->get().remainingQuantity(), Quantity(20));
}

TEST(MatchingEngineTest, PricePriorityMatchesBestPriceFirst) {
    MatchingEngine engine("AAPL");
    // Two sellers; the 99 seller should be matched before the 101 seller,
    // even though 101 was submitted first.
    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(101), Quantity(10)));
    engine.submitOrder(makeOrder(OrderId(2), Side::Sell, Price(99), Quantity(10)));

    auto trades = engine.submitOrder(makeOrder(OrderId(3), Side::Buy, Price(101), Quantity(10)));

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].sellOrderId(), OrderId(2)); // best price (99) wins, not first-arrived
    EXPECT_EQ(trades[0].price(), Price(99));
}

TEST(MatchingEngineTest, TimePriorityMatchesEarliestOrderFirstAtSamePrice) {
    MatchingEngine engine("AAPL");
    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(10)));
    engine.submitOrder(makeOrder(OrderId(2), Side::Sell, Price(100), Quantity(10)));

    auto trades = engine.submitOrder(makeOrder(OrderId(3), Side::Buy, Price(100), Quantity(10)));

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].sellOrderId(), OrderId(1)); // arrived first at same price, wins
}

TEST(MatchingEngineTest, LargeAggressiveOrderSweepsMultiplePriceLevels) {
    MatchingEngine engine("AAPL");

    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(10)));
    engine.submitOrder(makeOrder(OrderId(2), Side::Sell, Price(101), Quantity(10)));
    engine.submitOrder(makeOrder(OrderId(3), Side::Sell, Price(102), Quantity(10)));

    // Aggressive buy at 102 should sweep all executable price levels.
    auto trades = engine.submitOrder(makeOrder(OrderId(4), Side::Buy, Price(102), Quantity(25)));

    ASSERT_EQ(trades.size(), 3);

    EXPECT_EQ(trades[0].price(), Price(100));
    EXPECT_EQ(trades[0].quantity(), Quantity(10));

    EXPECT_EQ(trades[1].price(), Price(101));
    EXPECT_EQ(trades[1].quantity(), Quantity(10));

    EXPECT_EQ(trades[2].price(), Price(102));
    EXPECT_EQ(trades[2].quantity(), Quantity(5));

    // Five shares should remain from the last resting sell order.
    EXPECT_FALSE(engine.book().empty());

    ASSERT_TRUE(engine.book().bestAsk().has_value());
    EXPECT_EQ(*engine.book().bestAsk(), Price(102));

    auto remaining = engine.book().findOrder(OrderId(3));
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(remaining->get().remainingQuantity(), Quantity(5));

    EXPECT_FALSE(engine.book().bestBid().has_value()); // incoming buy fully consumed
}

TEST(MatchingEngineTest, NonCrossingOrderRestsWithoutTrading) {
    MatchingEngine engine("AAPL");
    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(105), Quantity(10)));

    auto trades = engine.submitOrder(makeOrder(OrderId(2), Side::Buy, Price(100), Quantity(10)));

    EXPECT_TRUE(trades.empty()); // 100 does not cross 105
    ASSERT_TRUE(engine.book().bestBid().has_value());
    EXPECT_EQ(*engine.book().bestBid(), Price(100));
    ASSERT_TRUE(engine.book().bestAsk().has_value());
    EXPECT_EQ(*engine.book().bestAsk(), Price(105));
}

TEST(MatchingEngineTest, CancelRemovesRestingOrder) {
    MatchingEngine engine("AAPL");
    engine.submitOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    EXPECT_TRUE(engine.cancelOrder(OrderId(1)));
    EXPECT_FALSE(engine.book().hasOrder(OrderId(1)));
}

TEST(MatchingEngineTest, CancelNonExistentOrderFails) {
    MatchingEngine engine("AAPL");
    EXPECT_FALSE(engine.cancelOrder(OrderId(999)));
}

TEST(MatchingEngineTest, ModifyQuantityDecreasePreservesTimePriority) {
    MatchingEngine engine("AAPL");
    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(20)));
    engine.submitOrder(makeOrder(OrderId(2), Side::Sell, Price(100), Quantity(20)));

    // Order 1 reduces its quantity - should NOT lose its place in line.
    auto result = engine.modifyOrder(OrderId(1), Price(100), Quantity(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty()); // pure decrease never triggers a match

    auto trades = engine.submitOrder(makeOrder(OrderId(3), Side::Buy, Price(100), Quantity(5)));
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].sellOrderId(), OrderId(1)); // still first in line despite modify
}

TEST(MatchingEngineTest, ModifyPriceForfeitsTimePriority) {
    MatchingEngine engine("AAPL");

    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(10)));
    engine.submitOrder(makeOrder(OrderId(2), Side::Sell, Price(100), Quantity(10)));

    // Move away from the price, then back.
    // This should reset Order 1's priority.
    engine.modifyOrder(OrderId(1), Price(101), Quantity(10));
    engine.modifyOrder(OrderId(1), Price(100), Quantity(10));

    auto trades = engine.submitOrder(makeOrder(OrderId(3), Side::Buy, Price(100), Quantity(10)));

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].sellOrderId(), OrderId(2));
}

TEST(MatchingEngineTest, ModifyNonExistentOrderReturnsNullopt) {
    MatchingEngine engine("AAPL");
    auto result = engine.modifyOrder(OrderId(999), Price(100), Quantity(10));
    EXPECT_FALSE(result.has_value());
}

TEST(MatchingEngineTest, ModifyThatCrossesBookGeneratesTrade) {
    MatchingEngine engine("AAPL");
    engine.submitOrder(makeOrder(OrderId(1), Side::Sell, Price(100), Quantity(10)));
    engine.submitOrder(
        makeOrder(OrderId(2), Side::Buy, Price(90), Quantity(10))); // rests, doesn't cross

    // Raise the buy price so it now crosses the resting sell at 100.
    auto result = engine.modifyOrder(OrderId(2), Price(100), Quantity(10));

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ((*result)[0].price(), Price(100));
    EXPECT_TRUE(engine.book().empty());
}