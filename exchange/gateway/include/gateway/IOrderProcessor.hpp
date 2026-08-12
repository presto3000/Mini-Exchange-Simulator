#pragma once

#include "common/Order.hpp"
#include "common/protocol/Messages.hpp"

#include <variant>

namespace exchange::gateway {

using ProcessResult =
    std::variant<common::protocol::OrderAckMessage, common::protocol::OrderRejectMessage>;

// Abstraction over "however an order actually gets handled" - Risk
// checks, then matching, wherever those live. The Gateway's networking
// code (Session/Server below) depends only on this interface, never on
// a concrete transport. In a real distributed RemoteOrderProcessor
// (Risk over TCP, Matching Engine over TCP) without touching a single line of socket-handling code here.
class IOrderProcessor {
public:
    virtual ~IOrderProcessor() = default;

    virtual ProcessResult submitOrder(common::Order order) = 0;
    virtual ProcessResult cancelOrder(common::OrderId id) = 0;
    virtual ProcessResult modifyOrder(common::OrderId id, common::Price newPrice,
                                      common::Quantity newQuantity) = 0;
};

} // namespace exchange::gateway