#pragma once

#include "common/MatchingEngine.hpp"
#include "common/protocol/Frame.hpp"
#include "common/protocol/Messages.hpp"

#include <functional>
#include <unordered_map>
#include <utility>

namespace exchange::matching_engine {

using TradeSink = std::function<void(const common::Trade&)>;

class MatchingDispatcher {
public:
    // 'sink' defaults to a no-op, so existing tests that
    // construct MatchingDispatcher with no arguments keep compiling and
    // behaving identically - publishing is additive, not a breaking
    // change to the dispatcher's core responsibility.
    explicit MatchingDispatcher(TradeSink sink = [](const common::Trade&) {})
        : tradeSink_(std::move(sink)) {}

    std::vector<std::byte> handle(const common::protocol::DecodedFrame& frame) {
        using namespace common::protocol;
        try {
            ByteReader reader(frame.payload);
            switch (frame.type) {
            case MessageType::NewOrder: {
                common::Order order = readOrder(reader);
                auto& engine = engineFor(order.symbol());
                auto trades = engine.submitOrder(std::move(order));
                publishAll(trades);
                return encodeAck(order.id(), std::move(trades));
            }
            case MessageType::ModifyOrder: {
                auto msg = readModifyOrder(reader);
                for (auto& [symbol, engine] : engines_) {
                    auto result = engine.modifyOrder(msg.orderId, msg.newPrice, msg.newQuantity);
                    if (result.has_value()) {
                        publishAll(*result);
                        return encodeAck(msg.orderId, std::move(*result));
                    }
                }
                return encodeReject(msg.orderId, "order not found");
            }
            case MessageType::CancelOrder: {
                auto msg = readCancelOrder(reader);
                for (auto& [symbol, engine] : engines_) {
                    if (engine.cancelOrder(msg.orderId)) {
                        return encodeAck(msg.orderId, {});
                    }
                }
                return encodeReject(msg.orderId, "order not found");
            }
            case MessageType::BookQuery: {
                auto msg = readBookQuery(reader);
                return encodeBookSnapshot(buildSnapshot(msg.symbol));
            }
            default:
                return encodeReject(common::OrderId(0), "unknown message type");
            }
        } catch (const std::out_of_range&) {
            return encodeReject(common::OrderId(0), "malformed message payload");
        }
    }

private:
    void publishAll(const std::vector<common::Trade>& trades) {
        for (const auto& trade : trades)
            tradeSink_(trade);
    }

    common::MatchingEngine& engineFor(const common::Symbol& symbol) {
        auto it = engines_.find(symbol);
        if (it == engines_.end())
            it = engines_.emplace(symbol, common::MatchingEngine(symbol)).first;
        return it->second;
    }

    static std::vector<std::byte> encodeAck(common::OrderId id, std::vector<common::Trade> trades) {
        common::protocol::ByteWriter w;
        common::protocol::writeOrderAck(w, {id, std::move(trades)});
        return common::protocol::encodeFrame(common::protocol::MessageType::OrderAck, w.bytes());
    }
    static std::vector<std::byte> encodeReject(common::OrderId id, const std::string& reason) {
        common::protocol::ByteWriter w;
        common::protocol::writeOrderReject(w, {id, reason});
        return common::protocol::encodeFrame(common::protocol::MessageType::OrderReject, w.bytes());
    }

    common::protocol::BookSnapshotMessage buildSnapshot(const common::Symbol& symbol) {
        using namespace common::protocol;
        BookSnapshotMessage snapshot;
        snapshot.symbol = symbol;

        auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            return snapshot; // no engine yet for this symbol = empty book, not an error
        }

        const auto& book = it->second.book();
        for (const auto& [price, level] : book.buyLevels()) {
            snapshot.bids.push_back(
                {price, level.totalQuantity(), static_cast<std::uint32_t>(level.orderCount())});
        }
        for (const auto& [price, level] : book.sellLevels()) {
            snapshot.asks.push_back(
                {price, level.totalQuantity(), static_cast<std::uint32_t>(level.orderCount())});
        }
        return snapshot;
    }

    static std::vector<std::byte>
    encodeBookSnapshot(const common::protocol::BookSnapshotMessage& snap) {
        common::protocol::ByteWriter w;
        common::protocol::writeBookSnapshot(w, snap);
        return common::protocol::encodeFrame(common::protocol::MessageType::BookSnapshot,
                                             w.bytes());
    }

    std::unordered_map<common::Symbol, common::MatchingEngine> engines_;
    TradeSink tradeSink_;
};

} // namespace exchange::matching_engine