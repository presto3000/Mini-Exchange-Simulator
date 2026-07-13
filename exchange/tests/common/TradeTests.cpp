#include <gtest/gtest.h>

#include "common/Trade.hpp"

using namespace exchange::common;

TEST(TradeTest, ConstructionSetsAllFields) {
    const auto ts = Timestamp{std::chrono::seconds(500)};
    const Trade trade(OrderId(1), OrderId(2), "AAPL", Price(2500), Quantity(50), ts);

    EXPECT_EQ(trade.buyOrderId(), OrderId(1));
    EXPECT_EQ(trade.sellOrderId(), OrderId(2));
    EXPECT_EQ(trade.symbol(), "AAPL");
    EXPECT_EQ(trade.price(), Price(2500));
    EXPECT_EQ(trade.quantity(), Quantity(50));
    EXPECT_EQ(trade.timestamp(), ts);
}

TEST(TradeTest, IsCopyable) {
    const Trade original(OrderId(1), OrderId(2), "AAPL", Price(2500), Quantity(50), Timestamp{});
    const Trade copy = original; // NOLINT - deliberately testing copy semantics

    EXPECT_EQ(copy.buyOrderId(), original.buyOrderId());
    EXPECT_EQ(copy.price(), original.price());
}