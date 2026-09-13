#pragma once

#include "common/net/SyncTcpClient.hpp"
#include "common/protocol/Messages.hpp"

#include <atomic>

namespace exchange::client {

// Wraps a SyncTcpClient connection to the Gateway, translating typed
// requests into wire frames and back. Owns the client-side OrderId
// generator - simplification rather than a production-grade ID scheme.
class ExchangeClient {
public:
    ExchangeClient(std::string host, unsigned short port) : tcp_(std::move(host), port) {
        tcp_.connect();
    }

    [[nodiscard]] common::OrderId nextOrderId() {
        return common::OrderId(nextId_.fetch_add(1));
    }

    common::protocol::DecodedFrame submitOrder(const common::Order& order) {
        common::protocol::ByteWriter w;
        common::protocol::writeOrder(w, order);
        return tcp_.sendAndReceive(
            common::protocol::encodeFrame(common::protocol::MessageType::NewOrder, w.bytes()));
    }

    common::protocol::DecodedFrame cancelOrder(common::OrderId id) {
        common::protocol::ByteWriter w;
        common::protocol::writeCancelOrder(w, {id});
        return tcp_.sendAndReceive(
            common::protocol::encodeFrame(common::protocol::MessageType::CancelOrder, w.bytes()));
    }

    common::protocol::DecodedFrame modifyOrder(common::OrderId id, common::Price price,
                                               common::Quantity qty) {
        common::protocol::ByteWriter w;
        common::protocol::writeModifyOrder(w, {id, price, qty});
        return tcp_.sendAndReceive(
            common::protocol::encodeFrame(common::protocol::MessageType::ModifyOrder, w.bytes()));
    }

    common::protocol::DecodedFrame queryBook(const common::Symbol& symbol) {
        common::protocol::ByteWriter w;
        common::protocol::writeBookQuery(w, {symbol});
        return tcp_.sendAndReceive(
            common::protocol::encodeFrame(common::protocol::MessageType::BookQuery, w.bytes()));
    }

private:
    common::net::SyncTcpClient tcp_;
    std::atomic<std::uint64_t> nextId_{1};
};

} // namespace exchange::client