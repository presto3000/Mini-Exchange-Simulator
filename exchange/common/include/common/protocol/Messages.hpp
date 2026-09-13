#pragma once

#include "common/Order.hpp"
#include "common/Trade.hpp"
#include "common/protocol/Serialization.hpp"

namespace exchange::common::protocol {

struct CancelOrderMessage {
    OrderId orderId;
};

struct ModifyOrderMessage {
    OrderId orderId;
    Price newPrice;
    Quantity newQuantity;
};

// Sent back to the client after a NewOrder/ModifyOrder is accepted.
// Carries every Trade generated (possibly zero, if the order simply
// rested without crossing).
struct OrderAckMessage {
    OrderId orderId;
    std::vector<Trade> trades;
};

// Sent back when an order is rejected, at either the Gateway's format
// validation stage or the Risk service's rule checks. 'reason' is a
// human-readable string (not just a numeric code) so the client console
// can print something directly useful to the trader
// without a separate lookup table.
struct OrderRejectMessage {
    OrderId orderId;
    std::string reason;
};

inline void writeCancelOrder(ByteWriter& w, const CancelOrderMessage& m) {
    w.writeUInt64(m.orderId.get());
}
inline CancelOrderMessage readCancelOrder(ByteReader& r) {
    return CancelOrderMessage{OrderId(r.readUInt64())};
}

inline void writeModifyOrder(ByteWriter& w, const ModifyOrderMessage& m) {
    w.writeUInt64(m.orderId.get());
    w.writeInt64(m.newPrice.get());
    w.writeInt64(m.newQuantity.get());
}
inline ModifyOrderMessage readModifyOrder(ByteReader& r) {
    const OrderId id(r.readUInt64());
    const Price price(r.readInt64());
    const Quantity qty(r.readInt64());
    return ModifyOrderMessage{id, price, qty};
}

inline void writeOrderAck(ByteWriter& w, const OrderAckMessage& m) {
    w.writeUInt64(m.orderId.get());
    w.writeUInt32(static_cast<std::uint32_t>(m.trades.size()));
    for (const auto& trade : m.trades) {
        writeTrade(w, trade);
    }
}
inline OrderAckMessage readOrderAck(ByteReader& r) {
    const OrderId id(r.readUInt64());
    const auto count = r.readUInt32();
    std::vector<Trade> trades;
    trades.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        trades.push_back(readTrade(r));
    }
    return OrderAckMessage{id, std::move(trades)};
}

inline void writeOrderReject(ByteWriter& w, const OrderRejectMessage& m) {
    w.writeUInt64(m.orderId.get());
    w.writeString(m.reason);
}
inline OrderRejectMessage readOrderReject(ByteReader& r) {
    const OrderId id(r.readUInt64());
    std::string reason = r.readString();
    return OrderRejectMessage{id, std::move(reason)};
}

// Published by Market Data after processing each trade. Deliberately
// approximate: bestBid/bestAsk here are derived from the last traded
// price only (see MarketDataStore's class comment for why), NOT from a
// real resting order book - Market Data never sees the order book
// directly per the spec ("Receives trade events"), only the trade
// stream. A genuine top-of-book feed is listed as a future improvement.
struct MarketDataSnapshotMessage {
    Symbol symbol;
    Price bestBid;
    Price bestAsk;
    Price lastPrice;
    Quantity lastQuantity;
    Timestamp timestamp;
};

inline void writeMarketDataSnapshot(ByteWriter& w, const MarketDataSnapshotMessage& m) {
    w.writeString(m.symbol);
    w.writeInt64(m.bestBid.get());
    w.writeInt64(m.bestAsk.get());
    w.writeInt64(m.lastPrice.get());
    w.writeInt64(m.lastQuantity.get());
    w.writeInt64(m.timestamp.time_since_epoch().count());
}
inline MarketDataSnapshotMessage readMarketDataSnapshot(ByteReader& r) {
    Symbol symbol = r.readString();
    const Price bestBid(r.readInt64());
    const Price bestAsk(r.readInt64());
    const Price lastPrice(r.readInt64());
    const Quantity lastQty(r.readInt64());
    const Timestamp::duration::rep ticks = r.readInt64();
    return MarketDataSnapshotMessage{std::move(symbol), bestBid,
                                     bestAsk,           lastPrice,
                                     lastQty,           Timestamp{Timestamp::duration(ticks)}};
}

struct BookQueryMessage {
    Symbol symbol;
};

// One resting price level's aggregate state, for display purposes only
// (mirrors PriceLevel::totalQuantity()/orderCount() -
// this is the cold-path "reporting" data those methods were explicitly
// designed to serve, now finally consumed).
struct PriceLevelSnapshot {
    Price price;
    Quantity totalQuantity;
    std::uint32_t orderCount;
};

struct BookSnapshotMessage {
    Symbol symbol;
    std::vector<PriceLevelSnapshot> bids; // best (highest) first
    std::vector<PriceLevelSnapshot> asks; // best (lowest) first
};

inline void writeBookQuery(ByteWriter& w, const BookQueryMessage& m) {
    w.writeString(m.symbol);
}
inline BookQueryMessage readBookQuery(ByteReader& r) {
    return BookQueryMessage{r.readString()};
}

inline void writeLevelSnapshot(ByteWriter& w, const PriceLevelSnapshot& lvl) {
    w.writeInt64(lvl.price.get());
    w.writeInt64(lvl.totalQuantity.get());
    w.writeUInt32(lvl.orderCount);
}
inline PriceLevelSnapshot readLevelSnapshot(ByteReader& r) {
    const Price price(r.readInt64());
    const Quantity qty(r.readInt64());
    const auto count = r.readUInt32();
    return PriceLevelSnapshot{price, qty, count};
}

inline void writeBookSnapshot(ByteWriter& w, const BookSnapshotMessage& m) {
    w.writeString(m.symbol);
    w.writeUInt32(static_cast<std::uint32_t>(m.bids.size()));
    for (const auto& lvl : m.bids)
        writeLevelSnapshot(w, lvl);
    w.writeUInt32(static_cast<std::uint32_t>(m.asks.size()));
    for (const auto& lvl : m.asks)
        writeLevelSnapshot(w, lvl);
}
inline BookSnapshotMessage readBookSnapshot(ByteReader& r) {
    Symbol symbol = r.readString();
    BookSnapshotMessage msg;
    msg.symbol = symbol;
    const auto bidCount = r.readUInt32();
    msg.bids.reserve(bidCount);
    for (std::uint32_t i = 0; i < bidCount; ++i)
        msg.bids.push_back(readLevelSnapshot(r));
    const auto askCount = r.readUInt32();
    msg.asks.reserve(askCount);
    for (std::uint32_t i = 0; i < askCount; ++i)
        msg.asks.push_back(readLevelSnapshot(r));
    return msg;
}

} // namespace exchange::common::protocol