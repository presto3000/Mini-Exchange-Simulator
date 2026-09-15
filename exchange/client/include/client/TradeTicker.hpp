#pragma once

#include "common/Logger.hpp"
#include "common/protocol/Serialization.hpp"

#include <atomic>
#include <iomanip>
#include <iostream>
#include <thread>
#include <zmq.hpp>

namespace exchange::client {

// Runs a background thread subscribed directly to Market Data's PUB
// feed and prints each incoming trade to the console. This is the
// concrete client-side design decision above: TRADES is a live stream, so it uses the same PUB/SUB
// transport Persistence and Market Data already use to talk to Matching Engine - the client is
// simply one more subscriber on that same architecture.
class TradeTicker {
public:
    TradeTicker(zmq::context_t& context, const std::string& marketDataHost, unsigned short port)
        : subscriber_(context, zmq::socket_type::sub) {
        subscriber_.connect("tcp://" + marketDataHost + ":" + std::to_string(port));
        subscriber_.set(zmq::sockopt::subscribe, ""); // all symbols
        subscriber_.set(zmq::sockopt::rcvtimeo, 200); // allows clean shutdown checks below
    }

    void start() {
        running_ = true;
        thread_ = std::thread([this] { run(); });
    }

    void stop() {
        running_ = false;
        if (thread_.joinable())
            thread_.join();
    }

    ~TradeTicker() {
        stop();
    }

private:
    void run() {
        while (running_) {
            zmq::message_t topicMsg, payloadMsg;
            auto r1 = subscriber_.recv(topicMsg, zmq::recv_flags::none);
            if (!r1)
                continue; // rcvtimeo elapsed with nothing received - loop, check running_ again
            auto r2 = subscriber_.recv(payloadMsg, zmq::recv_flags::none);
            if (!r2)
                continue;

            // This feed publishes MarketDataSnapshot frames,
            // not raw Trade frames - we read the snapshot's lastPrice/
            // lastQuantity, which is what the client actually cares about
            // displaying as "the tape."
            std::vector<std::byte> payload(static_cast<std::byte*>(payloadMsg.data()),
                                           static_cast<std::byte*>(payloadMsg.data()) +
                                               payloadMsg.size());
            auto decoded = common::protocol::tryDecodeFrame(payload);
            if (!decoded.has_value())
                continue;

            common::protocol::ByteReader reader(decoded->payload);
            auto snap = common::protocol::readMarketDataSnapshot(reader);

            std::cout << "[TRADE] " << snap.symbol << " price=" << snap.lastPrice.get()
                      << " qty=" << snap.lastQuantity.get() << "\n";
        }
    }

    zmq::socket_t subscriber_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace exchange::client