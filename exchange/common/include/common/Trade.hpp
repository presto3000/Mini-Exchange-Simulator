#pragma once

#include "common/Types.hpp"

namespace exchange::common {

// An immutable record of a completed match between two orders.
//
// Trade is intentionally immutable (all members const) - once a trade
// has happened, it is a historical fact. It will be published as an
// event to the Persistence and Market Data services, and
// nothing should ever be able to mutate a Trade after construction, since
// multiple services may hold copies of it concurrently.
class Trade {
public:
    Trade(OrderId buyOrderId, OrderId sellOrderId, Symbol symbol, Price price, Quantity quantity,
          Timestamp timestamp) noexcept
        : buyOrderId_(buyOrderId), sellOrderId_(sellOrderId), symbol_(std::move(symbol)),
          price_(price), quantity_(quantity), timestamp_(timestamp) {}

    [[nodiscard]] OrderId buyOrderId() const noexcept {
        return buyOrderId_;
    }
    [[nodiscard]] OrderId sellOrderId() const noexcept {
        return sellOrderId_;
    }
    [[nodiscard]] const Symbol& symbol() const noexcept {
        return symbol_;
    }
    [[nodiscard]] Price price() const noexcept {
        return price_;
    }
    [[nodiscard]] Quantity quantity() const noexcept {
        return quantity_;
    }
    [[nodiscard]] Timestamp timestamp() const noexcept {
        return timestamp_;
    }

private:
    const OrderId buyOrderId_;
    const OrderId sellOrderId_;
    const Symbol symbol_;
    const Price price_;
    const Quantity quantity_;
    const Timestamp timestamp_;
};

} // namespace exchange::common