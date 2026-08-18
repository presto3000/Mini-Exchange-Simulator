#pragma once

#include "common/net/FrameSession.hpp"

namespace exchange::common::net {

// Accepts connections and spawns a FrameSession per client, using
// whatever FrameHandler the owning service supplies. Identical role to
// gateway::Server, now shared by every service.
class FrameServer {
public:
    FrameServer(boost::asio::io_context& io, unsigned short port, FrameHandler handler,
                common::Logger& logger)
        : acceptor_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          handler_(std::move(handler)), logger_(logger) {
        acceptNext();
    }

private:
    void acceptNext() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    logger_.info("Accepted new connection.");
                    std::make_shared<FrameSession>(std::move(socket), handler_, logger_)->start();
                }
                acceptNext();
            });
    }

    boost::asio::ip::tcp::acceptor acceptor_;
    FrameHandler handler_;
    common::Logger& logger_;
};

} // namespace exchange::common::net