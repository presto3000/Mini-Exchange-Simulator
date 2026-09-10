#pragma once

#include "common/Trade.hpp"
#include "common/protocol/Serialization.hpp"

#include <zmq.hpp>

namespace exchange::matching_engine {

// Publishes every generated Trade over a ZeroMQ PUB socket. One
// TradePublisher per matching engine process, bound once at startup.
// Deliberately doesn't know or care whether zero, one, or many
// subscribers are attached (Persistence, Market Data, or future
// consumers) - that decoupling is the entire point of choosing PUB/SUB
// for this edge of the system back in Milestone 6.
class TradePublisher {
public:
    TradePublisher(zmq::context_t& context, const std::string& bindEndpoint)
        : socket_(context, zmq::socket_type::pub) {
        socket_.bind(bindEndpoint);
    }

    void publish(const common::Trade& trade) {
        common::protocol::ByteWriter writer;
        common::protocol::writeTrade(writer, trade);

        zmq::message_t topic(trade.symbol().begin(), trade.symbol().end());
        zmq::message_t payload(writer.bytes().begin(), writer.bytes().end());

        // sndmore on the topic frame - this is a two-part ZeroMQ message,
        // not two separate messages, so subscribers receive them as one
        // atomic unit via recv() called twice in sequence.
        socket_.send(topic, zmq::send_flags::sndmore);
        socket_.send(payload, zmq::send_flags::none);
    }

private:
    zmq::socket_t socket_;
};

} // namespace exchange::matching_engine