#pragma once

#include "gateway/Session.hpp"

namespace exchange::gateway {

// Accepts incoming client connections and spins up a Session for each.
// Owns the io_context's acceptor; does not own the IOrderProcessor
// (passed in by reference from main.cpp / tests) - dependency injection
// again, so tests can swap in whatever processor they need without
// touching Server itself.
class Server {
public:
    Server(boost::asio::io_context& io, unsigned short port, IOrderProcessor& processor,
           common::Logger& logger)
        : acceptor_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          processor_(processor), logger_(logger) {
        acceptNext();
    }

private:
    void acceptNext() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    logger_.info("Accepted new client connection.");
                    std::make_shared<Session>(std::move(socket), processor_, logger_)->start();
                }
                acceptNext(); // keep accepting further connections
            });
    }

    boost::asio::ip::tcp::acceptor acceptor_;
    IOrderProcessor& processor_;
    common::Logger& logger_;
};

} // namespace exchange::gateway