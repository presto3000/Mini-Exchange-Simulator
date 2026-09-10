#pragma once

#include "common/protocol/Serialization.hpp"
#include "persistence/ITradeRepository.hpp"

namespace exchange::persistence {

// Same testable seam as Market Data's TradeEventHandler: decodes a raw
// trade payload and hands it to whatever ITradeRepository was injected,
// with zero knowledge of ZeroMQ or Postgres.
class TradeEventHandler {
public:
    explicit TradeEventHandler(ITradeRepository& repository) : repository_(repository) {}

    void handleTradePayload(const std::vector<std::byte>& payload) {
        common::protocol::ByteReader reader(payload);
        common::Trade trade = common::protocol::readTrade(reader);
        repository_.save(trade);
    }

private:
    ITradeRepository& repository_;
};

} // namespace exchange::persistence