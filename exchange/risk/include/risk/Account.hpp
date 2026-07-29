#pragma once

#include "common/Types.hpp"

namespace exchange::risk {

// A deliberately simplified trading account: just an identifier and a
// buying power balance. Real accounts would track positions, margin,
// multiple currencies, etc. - out of scope per the spec's "simplified"
// requirement. This type exists so RiskChecker has something concrete
// to check against and mutate, without RiskChecker itself needing to
// know anything about how accounts are stored or looked up (that's a
// future AccountRepository's job, not this class's).
class Account {
public:
    Account(std::string accountId, common::BuyingPower buyingPower) noexcept
        : accountId_(std::move(accountId)), buyingPower_(buyingPower) {}

    [[nodiscard]] const std::string& accountId() const noexcept {
        return accountId_;
    }
    [[nodiscard]] common::BuyingPower buyingPower() const noexcept {
        return buyingPower_;
    }

    // Reduces available buying power by 'amount'. Precondition: caller
    // (RiskChecker) has already verified amount <= buyingPower_ - this
    // method does not re-check, since re-validating here would just
    // duplicate the check RiskChecker already performed a moment ago.
    void reserve(common::BuyingPower amount) noexcept {
        buyingPower_ = common::BuyingPower(buyingPower_.get() - amount.get());
    }

private:
    std::string accountId_;
    common::BuyingPower buyingPower_;
};

} // namespace exchange::risk