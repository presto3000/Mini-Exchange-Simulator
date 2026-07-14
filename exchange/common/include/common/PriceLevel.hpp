#pragma once

#include "common/Order.hpp"

#include <list>

namespace exchange::common {

// All resting orders at a single price, in strict time priority (FIFO).
//
// PriceLevel owns the orders physically (by value, in the list) so that
// the OrderBook's price-indexed map directly contains the order data -
// no extra heap indirection or shared ownership needed. Order objects
// are only ever moved between containers via std::list splicing or
// erased outright; they are never referenced by raw pointer from outside
// this structure (the OrderBook's side-index stores an *iterator*, not a
// pointer - see OrderBook.hpp for why that distinction matters).
class PriceLevel {
public:
    using OrderContainer = std::list<Order>;
    using iterator = OrderContainer::iterator;

    explicit PriceLevel(Price price) noexcept : price_(price) {}

    [[nodiscard]] Price price() const noexcept {
        return price_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return orders_.empty();
    }

    [[nodiscard]] std::size_t orderCount() const noexcept {
        return orders_.size();
    }

    // Total resting quantity at this price level (sum of remaining qty
    // across all orders). O(n) - only used for BOOK/snapshot display,
    // never on the matching hot path, so linear scan is acceptable here.
    [[nodiscard]] Quantity totalQuantity() const noexcept {
        std::int64_t total = 0;
        for (const auto& order : orders_) {
            total += order.remainingQuantity().get();
        }
        return Quantity(total);
    }

    // Appends a new order to the back of the queue (it has the lowest
    // time priority among orders currently at this price).
    iterator addOrder(Order order) {
        orders_.push_back(std::move(order));
        auto it = orders_.end();
        --it;
        return it;
    }

    // O(1): no shifting of other elements, per the container rationale
    // above.
    void removeOrder(iterator it) {
        orders_.erase(it);
    }

    [[nodiscard]] iterator begin() noexcept {
        return orders_.begin();
    }
    [[nodiscard]] iterator end() noexcept {
        return orders_.end();
    }
    [[nodiscard]] OrderContainer::const_iterator begin() const noexcept {
        return orders_.begin();
    }
    [[nodiscard]] OrderContainer::const_iterator end() const noexcept {
        return orders_.end();
    }

private:
    Price price_;
    OrderContainer orders_;
};

} // namespace exchange::common