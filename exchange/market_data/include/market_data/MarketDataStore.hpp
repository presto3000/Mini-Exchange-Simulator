#pragma once

#include "common/Trade.hpp"
#include "common/protocol/Messages.hpp"

#include <optional>
#include <unordered_map>

namespace exchange::market_data {

// Maintains a best-bid/best-ask approximation per symbol, derived
// PURELY from the trade event stream (per the spec: Market Data
// "receives trade events" - it never sees the real order book).
//
// Approximation used: after any trade, bestBid = bestAsk = that
// trade's price. This is a known, deliberate simplification - a real
// market data feed would show the actual best resting bid and ask,
// which requires seeing order book state directly, not just executed
// trades. Publishing genuine top-of-book quotes (as a second event type
// from the Matching Engine) is listed as a concrete future improvement

class MarketDataStore {
public:
    void onTrade(const common::Trade& trade) {
        auto& entry = snapshots_[trade.symbol()];
        entry.symbol = trade.symbol();
        entry.bestBid = trade.price();
        entry.bestAsk = trade.price();
        entry.lastPrice = trade.price();
        entry.lastQuantity = trade.quantity();
        entry.timestamp = trade.timestamp();
    }

    [[nodiscard]] std::optional<common::protocol::MarketDataSnapshotMessage>
    snapshot(const common::Symbol& symbol) const {
        auto it = snapshots_.find(symbol);
        if (it == snapshots_.end())
            return std::nullopt;
        return it->second;
    }

private:
    std::unordered_map<common::Symbol, common::protocol::MarketDataSnapshotMessage> snapshots_;
};

} // namespace exchange::market_data