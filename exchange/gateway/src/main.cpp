#include "common/Logger.hpp"
#include "common/net/FrameServer.hpp"
#include "gateway/InProcessOrderProcessor.hpp"
#include "gateway/MessageDispatcher.hpp"
#include "gateway/RemoteOrderProcessor.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
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
    common::Logger logger("gateway");
    logger.info("Gateway starting up.");

    const std::string mode = getEnvOr("GATEWAY_ORDER_PROCESSOR", "inprocess");

    std::unique_ptr<gateway::IOrderProcessor> processor;

    if (mode == "remote") {
        const std::string riskHost = getEnvOr("RISK_HOST", "localhost");
        const unsigned short riskPort =
            static_cast<unsigned short>(std::stoi(getEnvOr("RISK_PORT", "9200")));

        // Same startup-ordering issue as Risk->MatchingEngine: retry with
        // backoff rather than assuming Risk is already accepting
        // connections the instant this container starts.
        for (int attempt = 1; attempt <= 10 && !processor; ++attempt) {
            try {
                processor = std::make_unique<gateway::RemoteOrderProcessor>(riskHost, riskPort);
            } catch (const std::exception& e) {
                logger.warn("Risk service not ready (attempt " + std::to_string(attempt) +
                            "): " + e.what());
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }
        if (!processor) {
            logger.error("Failed to connect to Risk service after retries. Exiting.");
            return 1;
        }
        logger.info("Using RemoteOrderProcessor -> risk at " + riskHost + ":" +
                    std::to_string(riskPort));
    } else {
        risk::RiskConfig riskConfig{common::Quantity(10'000)};
        risk::Account account("demo-account", common::BuyingPower(1'000'000'000));
        processor = std::make_unique<gateway::InProcessOrderProcessor>(riskConfig, account);
        logger.info("Using InProcessOrderProcessor (standalone/dev mode).");
    }

    gateway::MessageDispatcher dispatcher(*processor);

    constexpr unsigned short port = 9000;
    boost::asio::io_context io;
    common::net::FrameServer server(
        io, port,
        [&dispatcher](const common::protocol::DecodedFrame& f) { return dispatcher.handle(f); },
        logger);

    logger.info("Listening on port 9000.");
    io.run();
    return 0;
}