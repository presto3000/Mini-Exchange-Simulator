#include "common/Logger.hpp"
#include "common/net/FrameServer.hpp"
#include "risk/RiskDispatcher.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

namespace {
std::string getEnvOr(const char* name, std::string fallback) {
    if (const char* v = std::getenv(name))
        return std::string(v);
    return fallback;
}
} // namespace

int main() {
    using namespace exchange;
    common::Logger logger("risk_service");
    logger.info("Risk service starting up.");

    const std::string meHost = getEnvOr("MATCHING_ENGINE_HOST", "localhost");
    const unsigned short mePort =
        static_cast<unsigned short>(std::stoi(getEnvOr("MATCHING_ENGINE_PORT", "9100")));

    risk::RiskConfig config{common::Quantity(10'000)};
    risk::Account account("demo-account", common::BuyingPower(1'000'000'000));

    // Retry connecting to the Matching Engine: in docker-compose,
    // 'depends_on' only controls container START order, not whether the
    // dependency's listener is actually accepting connections yet. A
    // fixed number of retries with backoff is a simple, honest fix for
    // that startup race - a production system would typically use a
    // proper healthcheck instead.
    std::unique_ptr<risk::RiskDispatcher> dispatcher;
    for (int attempt = 1; attempt <= 10 && !dispatcher; ++attempt) {
        try {
            dispatcher = std::make_unique<risk::RiskDispatcher>(config, account, meHost, mePort);
        } catch (const std::exception& e) {
            logger.warn("Matching Engine not ready (attempt " + std::to_string(attempt) +
                        "): " + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    if (!dispatcher) {
        logger.error("Failed to connect to Matching Engine after retries. Exiting.");
        return 1;
    }
    logger.info("Connected to Matching Engine at " + meHost + ":" + std::to_string(mePort));

    constexpr unsigned short port = 9200;
    boost::asio::io_context io;
    common::net::FrameServer server(
        io, port,
        [&dispatcher](const common::protocol::DecodedFrame& f) { return dispatcher->handle(f); },
        logger);

    logger.info("Listening on port 9200.");
    io.run();
    return 0;
}