#pragma once

#include "common/Order.hpp"
#include "risk/Account.hpp"
#include "risk/RiskConfig.hpp"

#include <string>

namespace exchange::risk {

enum class RiskOutcome {
    Accepted,
    RejectedNonPositivePrice,
    RejectedMaxQuantityExceeded,
    RejectedInsufficientBuyingPower
};

[[nodiscard]] inline std::string_view toString(RiskOutcome outcome) noexcept {
    switch (outcome) {
    case RiskOutcome::Accepted:
        return "ACCEPTED";
    case RiskOutcome::RejectedNonPositivePrice:
        return "REJECTED_NON_POSITIVE_PRICE";
    case RiskOutcome::RejectedMaxQuantityExceeded:
        return "REJECTED_MAX_QUANTITY_EXCEEDED";
    case RiskOutcome::RejectedInsufficientBuyingPower:
        return "REJECTED_INSUFFICIENT_BUYING_POWER";
    }
    return "UNKNOWN";
}

// Validates incoming orders against a fixed set of simplified risk
// rules before they are allowed to reach the MatchingEngine.
//
// RiskChecker holds only configuration (RiskConfig) - no account data,
// no order history, no global state. Every check is a pure function of
// its inputs (config + order + account), which is what makes this class
// trivially unit-testable and safe to reason about: given the same
// inputs, checkOrder always produces the same outcome.
class RiskChecker {
public:
    explicit RiskChecker(RiskConfig config) noexcept : config_(config) {}

    // Runs all risk checks in a fixed order. Checks are short-circuited
    // (first failure wins) rather than accumulated, matching how a real
    // pre-trade risk gate behaves: an order is either accepted whole or
    // rejected for one primary reason - traders need a single clear
    // rejection cause, not a list of every rule that happened to fail.
    //
    // On Accepted, mutates 'account' to reserve buying power for BUY
    // orders. On any rejection, 'account' is left untouched.
    [[nodiscard]] RiskOutcome checkOrder(const common::Order& order, Account& account) const {
        if (order.price().get() <= 0) {
            return RiskOutcome::RejectedNonPositivePrice;
        }

        if (order.originalQuantity().get() > config_.maxOrderQuantity.get()) {
            return RiskOutcome::RejectedMaxQuantityExceeded;
        }

        // Simplified model: only BUY orders consume buying power (a SELL
        // is assumed to be covered by an existing position - short-sell
        // risk is out of scope here, and explicitly left as a future
        // extension point alongside Market/Stop/Iceberg order types).
        if (order.side() == common::Side::Buy) {
            const std::int64_t cost = order.price().get() * order.originalQuantity().get();
            if (cost > account.buyingPower().get()) {
                return RiskOutcome::RejectedInsufficientBuyingPower;
            }
            account.reserve(common::BuyingPower(cost));
        }

        return RiskOutcome::Accepted;
    }

private:
    RiskConfig config_;
};

} // namespace exchange::risk