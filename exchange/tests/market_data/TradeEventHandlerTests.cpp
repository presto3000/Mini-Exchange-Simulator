#include <gtest/gtest.h>

#include "market_data/TradeEventHandler.hpp"

using namespace exchange::common;
using namespace exchange::common::protocol;
using namespace exchange::market_data;

TEST(TradeEventHandlerTest, ProducesDecodableSnapshotFrame) {
    MarketDataStore store;
    TradeEventHandler handler(store);

    Trade trade(OrderId(1), OrderId(2), "AAPL", Price(15000), Quantity(50), Timestamp{});
    ByteWriter w;
    writeTrade(w, trade);

    auto responseFrame = handler.handleTradePayload(w.bytes());
    auto decoded = tryDecodeFrame(responseFrame);

    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->type, MessageType::MarketDataSnapshot);

    ByteReader reader(decoded->payload);
    auto snapshot = readMarketDataSnapshot(reader);
    EXPECT_EQ(snapshot.symbol, "AAPL");
    EXPECT_EQ(snapshot.lastPrice, Price(15000));
}