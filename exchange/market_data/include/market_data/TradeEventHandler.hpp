#pragma once

#include "common/protocol/Serialization.hpp"
#include "market_data/MarketDataStore.hpp"
#include "common/protocol/Frame.hpp"

#include <vector>

namespace exchange::market_data {

// Decodes a raw trade payload (as received via ZeroMQ, topic frame
// already stripped by the caller) and applies it to the store,
// returning the encoded snapshot ready to re-publish. Kept free of any
// ZeroMQ types so it's testable with plain byte vectors - see
// TradeEventHandlerTests.cpp.
class TradeEventHandler {
public:
    explicit TradeEventHandler(MarketDataStore& store) : store_(store) {}

    std::vector<std::byte> handleTradePayload(const std::vector<std::byte>& payload) {
        common::protocol::ByteReader reader(payload);
        common::Trade trade = common::protocol::readTrade(reader);

        store_.onTrade(trade);

        auto snap = store_.snapshot(trade.symbol());
        common::protocol::ByteWriter writer;
        common::protocol::writeMarketDataSnapshot(writer, *snap);
        return common::protocol::encodeFrame(common::protocol::MessageType::MarketDataSnapshot,
                                             writer.bytes());
    }

private:
    MarketDataStore& store_;
};

} // namespace exchange::market_data