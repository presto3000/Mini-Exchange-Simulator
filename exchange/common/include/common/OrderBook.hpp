#pragma once

#include "common/PriceLevel.hpp"

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>

namespace exchange::common {

// Stores all resting orders for a single instrument (symbol), split into
// a buy side and a sell side, each maintained in strict price-time
// priority.
//
// One OrderBook instance exists per traded symbol. This class is
// deliberately *not* thread-safe internally - the MatchingEngine
// guarantees single-threaded access to any given
// OrderBook by design (see the MatchingEngine's class comment for the
// full explanation of why matching is single-threaded). Adding a mutex
// here would be paying a synchronization cost for a guarantee we
// already have at a higher level - false safety with a real
// performance cost.
class OrderBook {
public:
    explicit OrderBook(Symbol symbol) : symbol_(std::move(symbol)) {}

    [[nodiscard]] const Symbol& symbol() const noexcept {
        return symbol_;
    }

    // Inserts a new resting order into the correct side/price level.
    // O(log n) - one map lookup/insert for the price level (n = number
    // of distinct price levels, NOT number of orders), plus O(1) list
    // append. Also updates the side-index for O(1) future cancel/modify.
    void addOrder(Order order) {
        const OrderId id = order.id();
        const Side side = order.side();
        const Price price = order.price();

        PriceLevel::iterator orderIt;

        if (side == Side::Buy) {
            auto [levelIt, inserted] = buyLevels_.try_emplace(price, price);
            orderIt = levelIt->second.addOrder(std::move(order));
            index_.emplace(id, OrderLocation{side, price, orderIt});
        } else {
            auto [levelIt, inserted] = sellLevels_.try_emplace(price, price);
            orderIt = levelIt->second.addOrder(std::move(order));
            index_.emplace(id, OrderLocation{side, price, orderIt});
        }
    }

    // Removes an order by id. Returns false if the id is not present
    // (already filled/cancelled) - callers must handle that as a normal
    // case, not an error, since cancel racing against a fill is a
    // completely ordinary sequence of events.
    bool cancelOrder(OrderId id) {
        auto indexIt = index_.find(id);
        if (indexIt == index_.end()) {
            return false;
        }

        const OrderLocation& loc = indexIt->second;
        removeFromLevel(loc);
        index_.erase(indexIt);
        return true;
    }

    // Looks up a resting order's current data without removing it.
    // Used by MODIFY to read current state before
    // deciding whether time priority must reset.
    [[nodiscard]] std::optional<std::reference_wrapper<const Order>> findOrder(OrderId id) const {
        auto indexIt = index_.find(id);
        if (indexIt == index_.end()) {
            return std::nullopt;
        }
        return std::cref(*indexIt->second.orderIt);
    }

    [[nodiscard]] bool hasOrder(OrderId id) const noexcept {
        return index_.contains(id);
    }

    // Best (highest) buy price currently resting, if any.
    [[nodiscard]] std::optional<Price> bestBid() const noexcept {
        if (buyLevels_.empty())
            return std::nullopt;
        return buyLevels_.begin()->first;
    }

    // Best (lowest) sell price currently resting, if any.
    [[nodiscard]] std::optional<Price> bestAsk() const noexcept {
        if (sellLevels_.empty())
            return std::nullopt;
        return sellLevels_.begin()->first;
    }

    [[nodiscard]] bool empty() const noexcept {
        return buyLevels_.empty() && sellLevels_.empty();
    }

    // Exposed for MatchingEngine to walk price levels
    // during matching, and for BOOK snapshot display.
    using BuyLevels = std::map<Price, PriceLevel, std::greater<Price>>;
    using SellLevels = std::map<Price, PriceLevel, std::less<Price>>;

    [[nodiscard]] BuyLevels& buyLevels() noexcept {
        return buyLevels_;
    }
    [[nodiscard]] SellLevels& sellLevels() noexcept {
        return sellLevels_;
    }
    [[nodiscard]] const BuyLevels& buyLevels() const noexcept {
        return buyLevels_;
    }
    [[nodiscard]] const SellLevels& sellLevels() const noexcept {
        return sellLevels_;
    }

    // Removes an order from the side-index only, WITHOUT touching the
    // PriceLevel's list. Used exclusively by MatchingEngine when a
    // resting order has just been fully filled and already erased from
    // its PriceLevel directly (via the iterator matching already has in
    // hand) - this keeps the index in sync without a redundant second
    // lookup-and-erase through the normal cancelOrder() path.
    void forgetOrder(OrderId id) {
        index_.erase(id);
    }

    // Shrinks a resting order's quantity in place, preserving its
    // position (time priority) in its PriceLevel's queue. Used by
    // MODIFY when quantity decreases - per price-time priority rules,
    // a pure decrease does not forfeit queue position.
    bool modifyRestingOrderQuantity(OrderId id, Quantity newQuantity) {
        auto indexIt = index_.find(id);
        if (indexIt == index_.end()) {
            return false;
        }
        indexIt->second.orderIt->modify(indexIt->second.orderIt->price(), newQuantity);
        return true;
    }

private:
    struct OrderLocation {
        Side side;
        Price price;
        PriceLevel::iterator orderIt;
    };

    void removeFromLevel(const OrderLocation& loc) {
        if (loc.side == Side::Buy) {
            auto levelIt = buyLevels_.find(loc.price);
            levelIt->second.removeOrder(loc.orderIt);
            if (levelIt->second.empty()) {
                buyLevels_.erase(levelIt); // don't leak empty price levels
            }
        } else {
            auto levelIt = sellLevels_.find(loc.price);
            levelIt->second.removeOrder(loc.orderIt);
            if (levelIt->second.empty()) {
                sellLevels_.erase(levelIt);
            }
        }
    }

    Symbol symbol_;
    BuyLevels buyLevels_;
    SellLevels sellLevels_;
    std::unordered_map<OrderId, OrderLocation> index_;
};

} // namespace exchange::common