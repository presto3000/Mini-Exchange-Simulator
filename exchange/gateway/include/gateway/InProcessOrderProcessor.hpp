#pragma once

#include "common/MatchingEngine.hpp"
#include "gateway/IOrderProcessor.hpp"
#include "risk/Account.hpp"
#include "risk/RiskChecker.hpp"

#include <string>
#include <unordered_map>

namespace exchange::gateway {

// Concrete IOrderProcessor: wraps a RiskChecker and one
// MatchingEngine per symbol, all in-process (plain function calls, no
// sockets). This exists so the Gateway's networking layer is fully
// testable today, before Risk and Matching Engine exist as standalone
// services with their own listeners. It is explicitly a
// stepping stone, not a permanent architectural shortcut - the
// single-account, single-process simplification here would not hold up
// in the real distributed deployment, and is documented as such.
class InProcessOrderProcessor : public IOrderProcessor {
public:
    InProcessOrderProcessor(risk::RiskConfig riskConfig, risk::Account account)
        : riskChecker_(riskConfig), account_(std::move(account)) {}

    ProcessResult submitOrder(common::Order order) override {
        const auto outcome = riskChecker_.checkOrder(order, account_);
        if (outcome != risk::RiskOutcome::Accepted) {
            return common::protocol::OrderRejectMessage{order.id(),
                                                        std::string(risk::toString(outcome))};
        }

        auto& engine = engineFor(order.symbol());
        auto trades = engine.submitOrder(std::move(order));
        return common::protocol::OrderAckMessage{order.id(), std::move(trades)};
    }

    ProcessResult cancelOrder(common::OrderId id) override {
        // Simplified: I don't track which symbol an order id belongs to
        // at this layer (that index lives inside each MatchingEngine's
        // OrderBook). I try every known engine - fine for a portfolio
        // project's order volume; a real system would maintain an
        // id->symbol lookup to avoid the scan, worth flagging as a
        // future optimization if symbol count grows large.
        for (auto& [symbol, engine] : engines_) {
            if (engine.cancelOrder(id)) {
                return common::protocol::OrderAckMessage{id, {}};
            }
        }
        return common::protocol::OrderRejectMessage{id, "order not found"};
    }

    ProcessResult modifyOrder(common::OrderId id, common::Price newPrice,
                              common::Quantity newQuantity) override {
        for (auto& [symbol, engine] : engines_) {
            auto result = engine.modifyOrder(id, newPrice, newQuantity);
            if (result.has_value()) {
                return common::protocol::OrderAckMessage{id, std::move(*result)};
            }
        }
        return common::protocol::OrderRejectMessage{id, "order not found"};
    }

private:
    common::MatchingEngine& engineFor(const common::Symbol& symbol) {
        auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            it = engines_.emplace(symbol, common::MatchingEngine(symbol)).first;
        }
        return it->second;
    }

    risk::RiskChecker riskChecker_;
    risk::Account account_;
    std::unordered_map<common::Symbol, common::MatchingEngine> engines_;
};

} // namespace exchange::gateway