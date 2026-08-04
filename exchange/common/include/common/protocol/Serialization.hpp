#pragma once

#include "common/Order.hpp"
#include "common/Trade.hpp"
#include "common/protocol/ByteWriter.hpp"
#include "common/protocol/ByteReader.hpp"

#include <chrono>

namespace exchange::common::protocol {

// Encodes/decodes Order and Trade to/from the binary wire format.
// Deliberately free functions, not member functions on Order/Trade
// themselves - Order and Trade are pure domain types (common/) that
// should not know anything about wire formats; serialization is a
// protocol-layer concern layered on top, matching the same separation
// we've kept between OrderBook (structure) and MatchingEngine (logic).
inline void writeOrder(ByteWriter& w, const Order& order) {
    w.writeUInt64(order.id().get());
    w.writeString(order.symbol());
    w.writeUInt8(static_cast<std::uint8_t>(order.side()));
    w.writeInt64(order.price().get());
    w.writeInt64(order.originalQuantity().get());
    w.writeInt64(order.timestamp().time_since_epoch().count());
}

inline Order readOrder(ByteReader& r) {
    const OrderId id(r.readUInt64());
    Symbol symbol = r.readString();
    const auto side = static_cast<Side>(r.readUInt8());
    const Price price(r.readInt64());
    const Quantity qty(r.readInt64());
    const Timestamp::duration::rep ticks = r.readInt64();
    const Timestamp ts{Timestamp::duration(ticks)};

    return Order(id, std::move(symbol), side, price, qty, ts);
}

inline void writeTrade(ByteWriter& w, const Trade& trade) {
    w.writeUInt64(trade.buyOrderId().get());
    w.writeUInt64(trade.sellOrderId().get());
    w.writeString(trade.symbol());
    w.writeInt64(trade.price().get());
    w.writeInt64(trade.quantity().get());
    w.writeInt64(trade.timestamp().time_since_epoch().count());
}

inline Trade readTrade(ByteReader& r) {
    const OrderId buyId(r.readUInt64());
    const OrderId sellId(r.readUInt64());
    Symbol symbol = r.readString();
    const Price price(r.readInt64());
    const Quantity qty(r.readInt64());
    const Timestamp::duration::rep ticks = r.readInt64();
    const Timestamp ts{Timestamp::duration(ticks)};

    return Trade(buyId, sellId, std::move(symbol), price, qty, ts);
}




} // namespace exchange::common::protocol