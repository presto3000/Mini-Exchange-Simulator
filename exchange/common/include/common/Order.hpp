#pragma once

#include "common/Types.hpp"
#include <utility>
#include <cassert>

namespace exchange::common {

// Represents a single resting or incoming limit order.
//
// Design note: Order is a plain value type (no virtual functions, no
// inheritance). In a matching engine, orders are created and destroyed
// at extremely high frequency, and every order sits inside a container
// in the OrderBook. Virtual dispatch and heap-allocated
// polymorphism here would add vtable indirection and allocation overhead
// on the hottest path in the whole system. A flat, copyable struct lets
// the compiler reason about layout and lets us store orders by value in
// contiguous containers later - this is a deliberate low-latency choice,
// not an oversight of "missing OOP".
class Order {
public:
    Order(OrderId id, Symbol symbol, Side side, Price price, Quantity quantity,
          Timestamp timestamp) noexcept
        : id_(id), symbol_(std::move(symbol)), side_(side), price_(price), quantity_(quantity),
          remainingQuantity_(quantity), timestamp_(timestamp) {}

    [[nodiscard]] OrderId id() const noexcept {
        return id_;
    }
    [[nodiscard]] const Symbol& symbol() const noexcept {
        return symbol_;
    }
    [[nodiscard]] Side side() const noexcept {
        return side_;
    }
    [[nodiscard]] Price price() const noexcept {
        return price_;
    }
    [[nodiscard]] Quantity originalQuantity() const noexcept {
        return quantity_;
    }
    [[nodiscard]] Quantity remainingQuantity() const noexcept {
        return remainingQuantity_;
    }
    [[nodiscard]] Timestamp timestamp() const noexcept {
        return timestamp_;
    }

    [[nodiscard]] bool isFullyFilled() const noexcept {
        return remainingQuantity_.get() == 0;
    }

    // Reduces remaining quantity after a (partial) fill.
    // Precondition: fillQty <= remainingQuantity(). Violating this is a
    // matching-engine logic bug, not a user input error, so we assert
    // rather than throw - throwing would imply this is recoverable at
    // the call site, and it isn't.
    void applyFill(Quantity fillQty) noexcept {
        assert(fillQty.get() <= remainingQuantity_.get() &&
               "fill quantity cannot exceed remaining quantity");
        remainingQuantity_ = Quantity(remainingQuantity_.get() - fillQty.get());
    }

    // Used by MODIFY: replaces price/quantity in place.
    // Note: per price-time priority rules, a price change or a quantity
    // *increase* must reset time priority (the order goes to the back of
    // its new price level's queue). A quantity *decrease* keeps priority.
    // The MatchingEngine, not Order itself, is responsible
    // for enforcing that rule - Order just stores the new values.
    void modify(Price newPrice, Quantity newQuantity) noexcept {
        price_ = newPrice;
        quantity_ = newQuantity;
        remainingQuantity_ = newQuantity;
    }

private:
    OrderId id_;
    Symbol symbol_;
    Side side_;
    Price price_;
    Quantity quantity_;          // original quantity as submitted
    Quantity remainingQuantity_; // decreases as fills occur
    Timestamp timestamp_;        // used for time-priority within a price level
};

} // namespace exchange::common