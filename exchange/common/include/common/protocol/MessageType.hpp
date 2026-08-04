#pragma once

#include <cstdint>

namespace exchange::common::protocol {

// Identifies the payload that follows a frame's length prefix.
// Stored as a single byte on the wire - we have far fewer than 256
// message kinds, and every byte matters on a framing header that's sent
// once per message.
enum class MessageType : std::uint8_t {
    NewOrder = 1,
    CancelOrder = 2,
    ModifyOrder = 3,
    OrderAck = 4,
    OrderReject = 5,
    TradeEvent = 6,
};

} // namespace exchange::common::protocol