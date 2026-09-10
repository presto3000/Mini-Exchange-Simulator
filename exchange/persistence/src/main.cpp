#include "common/Logger.hpp"
#include "persistence/PostgresTradeRepository.hpp"
#include "persistence/TradeEventHandler.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
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
    common::Logger logger("persistence_service");
    logger.info("Persistence service starting up.");

    const std::string connStr = "host=" + getEnvOr("POSTGRES_HOST", "localhost") +
                                " port=" + getEnvOr("POSTGRES_PORT", "5432") +
                                " dbname=" + getEnvOr("POSTGRES_DB", "exchange") +
                                " user=" + getEnvOr("POSTGRES_USER", "exchange") +
                                " password=" + getEnvOr("POSTGRES_PASSWORD", "exchange");

    // Same startup-race reasoning as Risk->MatchingEngine:
    // Postgres's container starting does not mean it's ready to accept
    // connections yet.
    std::unique_ptr<persistence::PostgresTradeRepository> repository;
    for (int attempt = 1; attempt <= 10 && !repository; ++attempt) {
        try {
            repository = std::make_unique<persistence::PostgresTradeRepository>(connStr);
        } catch (const std::exception& e) {
            logger.warn("Postgres not ready (attempt " + std::to_string(attempt) +
                        "): " + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    if (!repository) {
        logger.error("Failed to connect to Postgres after retries. Exiting.");
        return 1;
    }
    logger.info("Connected to Postgres.");

    persistence::TradeEventHandler handler(*repository);

    const std::string meHost = getEnvOr("MATCHING_ENGINE_HOST", "localhost");
    const std::string mePort = getEnvOr("MATCHING_ENGINE_PUB_PORT", "9500");

    zmq::context_t zmqContext(1);
    zmq::socket_t subscriber(zmqContext, zmq::socket_type::sub);
    subscriber.connect("tcp://" + meHost + ":" + mePort);
    subscriber.set(zmq::sockopt::subscribe, "");

    logger.info("Subscribed to trades at " + meHost + ":" + mePort + ".");

    while (true) {
        zmq::message_t topicMsg, payloadMsg;
        auto r1 = subscriber.recv(topicMsg, zmq::recv_flags::none);
        auto r2 = subscriber.recv(payloadMsg, zmq::recv_flags::none);
        if (!r1 || !r2)
            continue;

        std::vector<std::byte> payload(static_cast<std::byte*>(payloadMsg.data()),
                                       static_cast<std::byte*>(payloadMsg.data()) +
                                           payloadMsg.size());
        handler.handleTradePayload(payload);
    }
}