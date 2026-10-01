
#include "common/Logger.hpp"
#include "common/net/FrameServer.hpp"
#include "matching_engine/KafkaTradePublisher.hpp"
#include "matching_engine/MatchingDispatcher.hpp"
#include "matching_engine/TradePublisher.hpp"

#include <cstdlib>

    namespace {
    std::string getEnvOr(const char* name, std::string fallback) {
        if (const char* v = std::getenv(name))
            return std::string(v);
        return fallback;
    }
}

int main() {
    using namespace exchange;
    common::Logger logger("matching_engine_service");
    logger.info("Matching Engine service starting up.");

    zmq::context_t zmqContext(1);
    matching_engine::TradePublisher zmqPublisher(zmqContext, "tcp://*:9500");

    const std::string kafkaBrokers = getEnvOr("KAFKA_BROKERS", "localhost:9092");
    matching_engine::KafkaTradePublisher kafkaPublisher(kafkaBrokers, "trades", logger);

    // A trade is a single domain event with two independent destinations
    // - the dispatcher doesn't know or care that there are two, it just
    // calls one sink. 
    matching_engine::MatchingDispatcher dispatcher(
        [&zmqPublisher, &kafkaPublisher](const common::Trade& trade) {
            zmqPublisher.publish(trade);
            kafkaPublisher.publish(trade);
        });

    constexpr unsigned short port = 9100;
    boost::asio::io_context io;
    common::net::FrameServer server(
        io, port,
        [&dispatcher](const common::protocol::DecodedFrame& f) { return dispatcher.handle(f); },
        logger);

    logger.info("Listening on port 9100. Publishing trades: ZeroMQ :9500, Kafka topic 'trades'.");
    io.run();
    return 0;
}