#include "common/Logger.hpp"
#include "gateway/InProcessOrderProcessor.hpp"
#include "gateway/Server.hpp"

int main() {
    using namespace exchange;

    common::Logger logger("gateway");
    logger.info("Gateway starting up.");

    // configuration: in-process Risk+MatchingEngine. See
    // InProcessOrderProcessor's class comment - next milestone replaces
    // this line with a RemoteOrderProcessor talking to standalone
    // risk_service / matching_engine_service processes, with no changes
    // needed below this line.
    risk::RiskConfig riskConfig{common::Quantity(10'000)};
    risk::Account account("demo-account", common::BuyingPower(1'000'000'000));
    gateway::InProcessOrderProcessor processor(riskConfig, account);

    constexpr unsigned short port = 9000;
    boost::asio::io_context io;
    gateway::Server server(io, port, processor, logger);

    logger.info("Listening on port 9000.");
    io.run(); // blocks, processing all async completions on this thread

    return 0;
}