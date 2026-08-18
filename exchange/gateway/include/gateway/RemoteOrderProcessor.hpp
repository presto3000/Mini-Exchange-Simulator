#pragma once

#include "common/net/SyncTcpClient.hpp"
#include "gateway/IOrderProcessor.hpp"

namespace exchange::gateway {

// IOrderProcessor: forwards every request over
// TCP to a standalone Risk service. The Gateway's networking code
// (FrameServer/FrameSession) required ZERO changes to support this - it
// only ever depended on IOrderProcessor, exactly as intended.
class RemoteOrderProcessor : public IOrderProcessor {
public:
    RemoteOrderProcessor(std::string riskHost, unsigned short riskPort)
        : riskClient_(std::move(riskHost), riskPort) {
        riskClient_.connect();
    }

    ProcessResult submitOrder(common::Order order) override {
        common::protocol::ByteWriter w;
        common::protocol::writeOrder(w, order);
        auto frame =
            common::protocol::encodeFrame(common::protocol::MessageType::NewOrder, w.bytes());
        return decodeResult(riskClient_.sendAndReceive(frame));
    }

    ProcessResult cancelOrder(common::OrderId id) override {
        common::protocol::ByteWriter w;
        common::protocol::writeCancelOrder(w, {id});
        auto frame =
            common::protocol::encodeFrame(common::protocol::MessageType::CancelOrder, w.bytes());
        return decodeResult(riskClient_.sendAndReceive(frame));
    }

    ProcessResult modifyOrder(common::OrderId id, common::Price newPrice,
                              common::Quantity newQuantity) override {
        common::protocol::ByteWriter w;
        common::protocol::writeModifyOrder(w, {id, newPrice, newQuantity});
        auto frame =
            common::protocol::encodeFrame(common::protocol::MessageType::ModifyOrder, w.bytes());
        return decodeResult(riskClient_.sendAndReceive(frame));
    }

private:
    static ProcessResult decodeResult(const common::protocol::DecodedFrame& frame) {
        using namespace common::protocol;
        ByteReader reader(frame.payload);
        if (frame.type == MessageType::OrderAck) {
            return readOrderAck(reader);
        }
        return readOrderReject(reader);
    }

    common::net::SyncTcpClient riskClient_;
};

} // namespace exchange::gateway