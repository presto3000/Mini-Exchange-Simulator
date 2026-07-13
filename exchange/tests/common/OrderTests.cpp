#include <gtest/gtest.h>

#include "common/Order.hpp"

using namespace exchange::common;

namespace {

Order makeOrder(std::int64_t qty, std::int64_t price = 100) {
    return Order(OrderId(1), "AAPL", Side::Buy, Price(price), Quantity(qty), Timestamp{});
}

} // namespace

TEST(OrderTest, ConstructionSetsAllFields) {
    const auto ts = Timestamp{std::chrono::seconds(1000)};
    const Order order(OrderId(42), "AAPL", Side::Sell, Price(2500), Quantity(10), ts);

    EXPECT_EQ(order.id(), OrderId(42));
    EXPECT_EQ(order.symbol(), "AAPL");
    EXPECT_EQ(order.side(), Side::Sell);
    EXPECT_EQ(order.price(), Price(2500));
    EXPECT_EQ(order.originalQuantity(), Quantity(10));
    EXPECT_EQ(order.remainingQuantity(), Quantity(10));
    EXPECT_EQ(order.timestamp(), ts);
}

TEST(OrderTest, NewOrderIsNotFullyFilled) {
    const Order order = makeOrder(100);
    EXPECT_FALSE(order.isFullyFilled());
}

TEST(OrderTest, PartialFillReducesRemainingQuantity) {
    Order order = makeOrder(100);
    order.applyFill(Quantity(40));

    EXPECT_EQ(order.remainingQuantity(), Quantity(60));
    EXPECT_EQ(order.originalQuantity(), Quantity(100)); // unchanged
    EXPECT_FALSE(order.isFullyFilled());
}

TEST(OrderTest, FullFillMarksOrderAsFilled) {
    Order order = makeOrder(100);
    order.applyFill(Quantity(100));

    EXPECT_EQ(order.remainingQuantity(), Quantity(0));
    EXPECT_TRUE(order.isFullyFilled());
}

TEST(OrderTest, MultiplePartialFillsAccumulate) {
    Order order = makeOrder(100);
    order.applyFill(Quantity(30));
    order.applyFill(Quantity(20));

    EXPECT_EQ(order.remainingQuantity(), Quantity(50));
}

TEST(OrderTest, ModifyReplacesPriceAndQuantityAndResetsRemaining) {
    Order order = makeOrder(100, /*price=*/100);
    order.applyFill(Quantity(40)); // remaining = 60

    order.modify(Price(105), Quantity(200));

    EXPECT_EQ(order.price(), Price(105));
    EXPECT_EQ(order.originalQuantity(), Quantity(200));
    EXPECT_EQ(order.remainingQuantity(), Quantity(200)); // reset, not 160
}