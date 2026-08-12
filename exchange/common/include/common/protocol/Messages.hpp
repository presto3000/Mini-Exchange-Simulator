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

} // namespace exchange::common::protocol