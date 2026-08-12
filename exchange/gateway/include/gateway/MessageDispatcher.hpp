#pragma once

#include "common/protocol/Frame.hpp"
#include "common/protocol/Messages.hpp"
#include "gateway/IOrderProcessor.hpp"

namespace exchange::gateway {

// Decodes one already-framed message and dispatches it to an
// IOrderProcessor, then encodes the response back into a frame ready to
// write to a socket. Kept fully separate from Boost.Asio - this class
// knows nothing about sockets, only about frames and the processor
// interface, which is what makes it testable without a network stack at
// all (see MessageDispatcherTests.cpp).
class MessageDispatcher {
public:
    explicit MessageDispatcher(IOrderProcessor& processor) : processor_(processor) {}

    // Returns the response frame to send back to the client. On a
    // message-format problem (unreadable payload, unknown type), this
    // is where "validate message format" from the Gateway's spec
    // responsibility is enforced - format errors are rejected here,
    // before ever reaching the processor/risk layer.
    [[nodiscard]] std::vector<std::byte> handle(const common::protocol::DecodedFrame& frame) {
        using namespace common::protocol;

        try {
            ByteReader reader(frame.payload);

            switch (frame.type) {
            case MessageType::NewOrder: {
                common::Order order = readOrder(reader);
                return encodeResult(processor_.submitOrder(std::move(order)));
            }
            case MessageType::CancelOrder: {
                auto msg = readCancelOrder(reader);
                return encodeResult(processor_.cancelOrder(msg.orderId));
            }
            case MessageType::ModifyOrder: {
                auto msg = readModifyOrder(reader);
                return encodeResult(
                    processor_.modifyOrder(msg.orderId, msg.newPrice, msg.newQuantity));
            }
            default:
                return encodeReject(common::OrderId(0), "unknown message type");
            }
        } catch (const std::out_of_range&) {
            // Thrown by ByteReader on a truncated/malformed
            // payload - this is exactly the "validate message format"
            // responsibility the spec assigns to the Gateway.
            return encodeReject(common::OrderId(0), "malformed message payload");
        }
    }

private:
    static std::vector<std::byte> encodeResult(const ProcessResult& result) {
        using namespace common::protocol;
        return std::visit(
            [](const auto& msg) -> std::vector<std::byte> {
                using T = std::decay_t<decltype(msg)>;
                ByteWriter w;
                if constexpr (std::is_same_v<T, OrderAckMessage>) {
                    writeOrderAck(w, msg);
                    return encodeFrame(MessageType::OrderAck, w.bytes());
                } else {
                    writeOrderReject(w, msg);
                    return encodeFrame(MessageType::OrderReject, w.bytes());
                }
            },
            result);
    }

    static std::vector<std::byte> encodeReject(common::OrderId id, const std::string& reason) {
        common::protocol::ByteWriter w;
        common::protocol::writeOrderReject(w, {id, reason});
        return common::protocol::encodeFrame(common::protocol::MessageType::OrderReject, w.bytes());
    }

    IOrderProcessor& processor_;
};

} // namespace exchange::gateway