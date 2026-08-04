#include <gtest/gtest.h>

#include "common/protocol/Serialization.hpp"

using namespace exchange::common;
using namespace exchange::common::protocol;

TEST(SerializationTest, OrderRoundTripsExactly) {
    const Order original(OrderId(42), "AAPL", Side::Buy, Price(2550), Quantity(100),
                         Timestamp{std::chrono::seconds(12345)});

    ByteWriter writer;
    writeOrder(writer, original);

    ByteReader reader(writer.bytes());
    Order decoded = readOrder(reader);

    EXPECT_EQ(decoded.id(), original.id());
    EXPECT_EQ(decoded.symbol(), original.symbol());
    EXPECT_EQ(decoded.side(), original.side());
    EXPECT_EQ(decoded.price(), original.price());
    EXPECT_EQ(decoded.originalQuantity(), original.originalQuantity());
    EXPECT_EQ(decoded.timestamp(), original.timestamp());
}

TEST(SerializationTest, TradeRoundTripsExactly) {
    const Trade original(OrderId(1), OrderId(2), "MSFT", Price(30000), Quantity(75),
                         Timestamp{std::chrono::seconds(999)});

    ByteWriter writer;
    writeTrade(writer, original);

    ByteReader reader(writer.bytes());
    Trade decoded = readTrade(reader);

    EXPECT_EQ(decoded.buyOrderId(), original.buyOrderId());
    EXPECT_EQ(decoded.sellOrderId(), original.sellOrderId());
    EXPECT_EQ(decoded.symbol(), original.symbol());
    EXPECT_EQ(decoded.price(), original.price());
    EXPECT_EQ(decoded.quantity(), original.quantity());
    EXPECT_EQ(decoded.timestamp(), original.timestamp());
}

TEST(SerializationTest, ReadingPastEndOfBufferThrows) {
    ByteWriter writer;
    writer.writeUInt8(1); // only one byte written

    ByteReader reader(writer.bytes());
    reader.readUInt8();                                   // fine, consumes the one byte
    EXPECT_THROW(reader.readUInt64(), std::out_of_range); // nothing left
}