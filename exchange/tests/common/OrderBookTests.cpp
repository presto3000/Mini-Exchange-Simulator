#include <gtest/gtest.h>

#include "common/OrderBook.hpp"

using namespace exchange::common;

namespace {
Order makeOrder(OrderId id, Side side, Price price, Quantity qty) {
    return Order(id, "AAPL", side, price, qty, Timestamp{});
}
} // namespace

TEST(OrderBookTest, EmptyBookHasNoBestPrices) {
    OrderBook book("AAPL");
    EXPECT_FALSE(book.bestBid().has_value());
    EXPECT_FALSE(book.bestAsk().has_value());
    EXPECT_TRUE(book.empty());
}

TEST(OrderBookTest, BestBidIsHighestBuyPrice) {
    OrderBook book("AAPL");
    book.addOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));
    book.addOrder(makeOrder(OrderId(2), Side::Buy, Price(105), Quantity(10)));
    book.addOrder(makeOrder(OrderId(3), Side::Buy, Price(102), Quantity(10)));

    ASSERT_TRUE(book.bestBid().has_value());
    EXPECT_EQ(*book.bestBid(), Price(105));
}

TEST(OrderBookTest, BestAskIsLowestSellPrice) {
    OrderBook book("AAPL");
    book.addOrder(makeOrder(OrderId(1), Side::Sell, Price(110), Quantity(10)));
    book.addOrder(makeOrder(OrderId(2), Side::Sell, Price(103), Quantity(10)));
    book.addOrder(makeOrder(OrderId(3), Side::Sell, Price(107), Quantity(10)));

    ASSERT_TRUE(book.bestAsk().has_value());
    EXPECT_EQ(*book.bestAsk(), Price(103));
}

TEST(OrderBookTest, CancelExistingOrderSucceedsAndRemovesIt) {
    OrderBook book("AAPL");
    book.addOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    EXPECT_TRUE(book.hasOrder(OrderId(1)));
    EXPECT_TRUE(book.cancelOrder(OrderId(1)));
    EXPECT_FALSE(book.hasOrder(OrderId(1)));
}

TEST(OrderBookTest, CancelNonExistentOrderReturnsFalse) {
    OrderBook book("AAPL");
    EXPECT_FALSE(book.cancelOrder(OrderId(999)));
}

TEST(OrderBookTest, CancelingLastOrderAtPriceRemovesTheLevel) {
    OrderBook book("AAPL");
    book.addOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    ASSERT_TRUE(book.bestBid().has_value());
    book.cancelOrder(OrderId(1));

    EXPECT_FALSE(book.bestBid().has_value()); // level cleaned up, not stale
}

TEST(OrderBookTest, BestBidUpdatesAfterCancelingTopOfBook) {
    OrderBook book("AAPL");
    book.addOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));
    book.addOrder(makeOrder(OrderId(2), Side::Buy, Price(105), Quantity(10)));

    book.cancelOrder(OrderId(2)); // cancel the best price

    ASSERT_TRUE(book.bestBid().has_value());
    EXPECT_EQ(*book.bestBid(), Price(100)); // falls back to next best
}

TEST(OrderBookTest, FindOrderReturnsCurrentData) {
    OrderBook book("AAPL");
    book.addOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));

    auto found = book.findOrder(OrderId(1));
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get().price(), Price(100));
}

TEST(OrderBookTest, FindOrderReturnsNulloptForMissingOrder) {
    OrderBook book("AAPL");
    EXPECT_FALSE(book.findOrder(OrderId(1)).has_value());
}

TEST(OrderBookTest, MultipleOrdersSamePriceMaintainFIFOWithinLevel) {
    OrderBook book("AAPL");
    book.addOrder(makeOrder(OrderId(1), Side::Buy, Price(100), Quantity(10)));
    book.addOrder(makeOrder(OrderId(2), Side::Buy, Price(100), Quantity(20)));

    auto& levels = book.buyLevels();
    auto levelIt = levels.find(Price(100));
    ASSERT_NE(levelIt, levels.end());

    auto orderIt = levelIt->second.begin();
    EXPECT_EQ(orderIt->id(), OrderId(1)); // arrived first, first in queue
}