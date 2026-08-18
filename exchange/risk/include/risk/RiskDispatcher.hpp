#pragma once

#include "common/net/SyncTcpClient.hpp"
#include "common/protocol/Messages.hpp"
#include "risk/Account.hpp"
#include "risk/RiskChecker.hpp"

namespace exchange::risk {

// Server-side handler for the standalone Risk service. Runs pre-trade
// checks on NewOrder messages; forwards risk-approved orders (and all
// Cancel/Modify messages, which don't need a fresh check) on to the
// Matching Engine service over its own SyncTcpClient connection.
//
// Holds exactly one demo Account for now - a per-client account
// lookup is a clearly separable future extension (see README), not
// something current distribution goal needs to solve.
class RiskDispatcher {
public:
    RiskDispatcher(RiskConfig config, Account account, std::string matchingEngineHost,
                   unsigned short matchingEnginePort)
        : checker_(config), account_(std::move(account)),
          matchingEngineClient_(std::move(matchingEngineHost), matchingEnginePort) {
        matchingEngineClient_.connect();
    }

    std::vector<std::byte> handle(const common::protocol::DecodedFrame& frame) {
        using namespace common::protocol;
        try {
            ByteReader reader(frame.payload);
            switch (frame.type) {
            case MessageType::NewOrder: {
                common::Order order = readOrder(reader);
                const auto outcome = checker_.checkOrder(order, account_);
                if (outcome != RiskOutcome::Accepted) {
                    return encodeReject(order.id(), std::string(toString(outcome)));
                }
                return forwardToMatchingEngine(frame);
            }
            case MessageType::CancelOrder:
            case MessageType::ModifyOrder:
                // Passed through unchanged; a stricter system might
                // re-check buying power on a quantity-increasing
                // modify - flagged as a future improvement.
                return forwardToMatchingEngine(frame);
            default:
                return encodeReject(common::OrderId(0), "unknown message type");
            }
        } catch (const std::out_of_range&) {
            return encodeReject(common::OrderId(0), "malformed message payload");
        }
    }

private:
    std::vector<std::byte> forwardToMatchingEngine(const common::protocol::DecodedFrame& frame) {
        auto requestFrame = common::protocol::encodeFrame(frame.type, frame.payload);
        auto decoded = matchingEngineClient_.sendAndReceive(requestFrame);
        return common::protocol::encodeFrame(decoded.type, decoded.payload);
    }

    static std::vector<std::byte> encodeReject(common::OrderId id, const std::string& reason) {
        common::protocol::ByteWriter w;
        common::protocol::writeOrderReject(w, {id, reason});
        return common::protocol::encodeFrame(common::protocol::MessageType::OrderReject, w.bytes());
    }

    RiskChecker checker_;
    Account account_;
    common::net::SyncTcpClient matchingEngineClient_;
};

} // namespace exchange::risk