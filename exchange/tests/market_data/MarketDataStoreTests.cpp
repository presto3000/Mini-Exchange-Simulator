#include <gtest/gtest.h>

#include "market_data/MarketDataStore.hpp"

using namespace exchange::common;
using namespace exchange::market_data;

TEST(MarketDataStoreTest, NoSnapshotBeforeAnyTrade) {
    MarketDataStore store;
    EXPECT_FALSE(store.snapshot("AAPL").has_value());
}

TEST(MarketDataStoreTest, TradeUpdatesSnapshotForItsSymbol) {
    MarketDataStore store;
    Trade trade(OrderId(1), OrderId(2), "AAPL", Price(15000), Quantity(50), Timestamp{});
    store.onTrade(trade);

    auto snap = store.snapshot("AAPL");
    ASSERT_TRUE(snap.has_value());
    EXPECT_EQ(snap->bestBid, Price(15000));
    EXPECT_EQ(snap->bestAsk, Price(15000));
    EXPECT_EQ(snap->lastPrice, Price(15000));
    EXPECT_EQ(snap->lastQuantity, Quantity(50));
}

TEST(MarketDataStoreTest, LaterTradeOverwritesEarlierSnapshotForSameSymbol) {
    MarketDataStore store;
    store.onTrade(Trade(OrderId(1), OrderId(2), "AAPL", Price(15000), Quantity(50), Timestamp{}));
    store.onTrade(Trade(OrderId(3), OrderId(4), "AAPL", Price(15100), Quantity(20), Timestamp{}));

    auto snap = store.snapshot("AAPL");
    ASSERT_TRUE(snap.has_value());
    EXPECT_EQ(snap->lastPrice, Price(15100));
    EXPECT_EQ(snap->lastQuantity, Quantity(20));
}

TEST(MarketDataStoreTest, DifferentSymbolsMaintainIndependentSnapshots) {
    MarketDataStore store;
    store.onTrade(Trade(OrderId(1), OrderId(2), "AAPL", Price(15000), Quantity(50), Timestamp{}));
    store.onTrade(Trade(OrderId(3), OrderId(4), "MSFT", Price(30000), Quantity(10), Timestamp{}));

    EXPECT_EQ(store.snapshot("AAPL")->lastPrice, Price(15000));
    EXPECT_EQ(store.snapshot("MSFT")->lastPrice, Price(30000));
}