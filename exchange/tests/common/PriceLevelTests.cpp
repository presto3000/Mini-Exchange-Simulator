#include <gtest/gtest.h>

#include "common/PriceLevel.hpp"

using namespace exchange::common;

namespace {
Order makeOrder(OrderId id, Quantity qty) {
    return Order(id, "AAPL", Side::Buy, Price(100), qty, Timestamp{});
}
} // namespace

TEST(PriceLevelTest, NewLevelIsEmpty) {
    PriceLevel level(Price(100));
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.orderCount(), 0);
}

TEST(PriceLevelTest, AddOrderIncreasesCount) {
    PriceLevel level(Price(100));
    level.addOrder(makeOrder(OrderId(1), Quantity(10)));

    EXPECT_FALSE(level.empty());
    EXPECT_EQ(level.orderCount(), 1);
}

TEST(PriceLevelTest, OrdersPreserveFIFOInsertionOrder) {
    PriceLevel level(Price(100));
    level.addOrder(makeOrder(OrderId(1), Quantity(10)));
    level.addOrder(makeOrder(OrderId(2), Quantity(20)));
    level.addOrder(makeOrder(OrderId(3), Quantity(30)));

    auto it = level.begin();
    EXPECT_EQ(it->id(), OrderId(1));
    ++it;
    EXPECT_EQ(it->id(), OrderId(2));
    ++it;
    EXPECT_EQ(it->id(), OrderId(3));
}

TEST(PriceLevelTest, TotalQuantitySumsAllOrders) {
    PriceLevel level(Price(100));
    level.addOrder(makeOrder(OrderId(1), Quantity(10)));
    level.addOrder(makeOrder(OrderId(2), Quantity(25)));

    EXPECT_EQ(level.totalQuantity(), Quantity(35));
}

TEST(PriceLevelTest, RemoveOrderIsO1AndPreservesOtherOrders) {
    PriceLevel level(Price(100));
    level.addOrder(makeOrder(OrderId(1), Quantity(10)));
    auto it2 = level.addOrder(makeOrder(OrderId(2), Quantity(20)));
    level.addOrder(makeOrder(OrderId(3), Quantity(30)));

    level.removeOrder(it2);

    EXPECT_EQ(level.orderCount(), 2);
    auto it = level.begin();
    EXPECT_EQ(it->id(), OrderId(1));
    ++it;
    EXPECT_EQ(it->id(), OrderId(3)); // order 2 gone, 1 and 3 remain in order
}