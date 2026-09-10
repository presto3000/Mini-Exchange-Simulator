#include "common/Logger.hpp"
#include "market_data/TradeEventHandler.hpp"

#include <cstdlib>
#include <string>
#include <zmq.hpp>

namespace {
std::string getEnvOr(const char* name, std::string fallback) {
    if (const char* v = std::getenv(name))
        return std::string(v);
    return fallback;
}
} // namespace

int main() {
    using namespace exchange;
    common::Logger logger("market_data_service");
    logger.info("Market Data service starting up.");

    const std::string meHost = getEnvOr("MATCHING_ENGINE_HOST", "localhost");
    const std::string mePort = getEnvOr("MATCHING_ENGINE_PUB_PORT", "9500");

    zmq::context_t zmqContext(1);

    zmq::socket_t subscriber(zmqContext, zmq::socket_type::sub);
    subscriber.connect("tcp://" + meHost + ":" + mePort);
    subscriber.set(zmq::sockopt::subscribe, ""); // all topics/symbols

    zmq::socket_t publisher(zmqContext, zmq::socket_type::pub);
    publisher.bind("tcp://*:9600");

    market_data::MarketDataStore store;
    market_data::TradeEventHandler handler(store);

    logger.info("Subscribed to trades at " + meHost + ":" + mePort +
                ". Publishing snapshots on port 9600.");

    // Blocking receive loop: this service has no client-facing request/
    // response responsibility (unlike Gateway/Risk/Matching Engine), so
    // there's no need for Boost.Asio here at all - a plain loop over
    // blocking ZeroMQ recv calls is the simplest correct implementation
    // for a pure event consumer/republisher.
    while (true) {
        zmq::message_t topicMsg, payloadMsg;
        auto r1 = subscriber.recv(topicMsg, zmq::recv_flags::none);
        auto r2 = subscriber.recv(payloadMsg, zmq::recv_flags::none);
        if (!r1 || !r2)
            continue;

        std::vector<std::byte> payload(static_cast<std::byte*>(payloadMsg.data()),
                                       static_cast<std::byte*>(payloadMsg.data()) +
                                           payloadMsg.size());

        auto snapshotFrame = handler.handleTradePayload(payload);

        zmq::message_t topicOut(topicMsg.data(), topicMsg.size());
        zmq::message_t payloadOut(snapshotFrame.data(), snapshotFrame.size());
        publisher.send(topicOut, zmq::send_flags::sndmore);
        publisher.send(payloadOut, zmq::send_flags::none);
    }
}