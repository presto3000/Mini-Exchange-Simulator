#include "common/Logger.hpp"
#include "common/net/FrameServer.hpp"
#include "matching_engine/MatchingDispatcher.hpp"

int main() {
    using namespace exchange;
    common::Logger logger("matching_engine_service");
    logger.info("Matching Engine service starting up.");

    matching_engine::MatchingDispatcher dispatcher;

    constexpr unsigned short port = 9100;
    boost::asio::io_context io;
    common::net::FrameServer server(
        io, port,
        [&dispatcher](const common::protocol::DecodedFrame& f) { return dispatcher.handle(f); },
        logger);

    logger.info("Listening on port 9100.");
    io.run();
    return 0;
}