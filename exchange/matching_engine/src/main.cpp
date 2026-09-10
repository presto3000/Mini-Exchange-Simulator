#include "common/Logger.hpp"
#include "common/net/FrameServer.hpp"
#include "matching_engine/MatchingDispatcher.hpp"
#include "matching_engine/TradePublisher.hpp"

int main() {
    using namespace exchange;
    common::Logger logger("matching_engine_service");
    logger.info("Matching Engine service starting up.");

    zmq::context_t zmqContext(1);
    matching_engine::TradePublisher tradePublisher(zmqContext, "tcp://*:9500");

    matching_engine::MatchingDispatcher dispatcher(
        [&tradePublisher](const common::Trade& trade) { tradePublisher.publish(trade); });

    constexpr unsigned short port = 9100;
    boost::asio::io_context io;
    common::net::FrameServer server(
        io, port,
        [&dispatcher](const common::protocol::DecodedFrame& f) { return dispatcher.handle(f); },
        logger);

    logger.info("Listening on port 9100. Publishing trades on port 9500.");
    io.run();
    return 0;
}