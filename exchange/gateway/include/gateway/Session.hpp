#pragma once

#include "common/Logger.hpp"
#include "gateway/MessageDispatcher.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <vector>

namespace exchange::gateway {

// Manages one connected client's TCP socket for its entire lifetime.
// One Session per connection - this is the standard Asio pattern
// (shared_ptr-owned, self-perpetuating async chain) that lets the
// io_context handle arbitrarily many concurrent clients without us
// managing threads per connection ourselves.
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, IOrderProcessor& processor, common::Logger& logger)
        : socket_(std::move(socket)), dispatcher_(processor), logger_(logger) {}

    void start() {
        readMore();
    }

private:
    void readMore() {
        auto self = shared_from_this();
        socket_.async_read_some(
            boost::asio::buffer(readChunk_),
            [this, self](boost::system::error_code ec, std::size_t bytesRead) {
                if (ec) {
                    logger_.info("Client disconnected: " + ec.message());
                    return; // session ends; shared_ptr chain unwinds, socket closes
                }

                inbox_.insert(inbox_.end(), reinterpret_cast<std::byte*>(readChunk_.data()),
                              reinterpret_cast<std::byte*>(readChunk_.data()) + bytesRead);

                processCompleteFrames();
                readMore(); // keep the async chain alive for this connection
            });
    }

    void processCompleteFrames() {
        // A single async_read_some can deliver a partial message, a
        // single complete message, or several complete messages back to
        // back - this loop drains every fully-available frame from the
        // buffer before returning to wait for more bytes, exactly the
        // "buffer, try-decode, consume, retry" pattern proven in
        // FrameTests.cpp.
        while (auto decoded = common::protocol::tryDecodeFrame(inbox_)) {
            auto response = dispatcher_.handle(*decoded);
            inbox_.erase(inbox_.begin(),
                         inbox_.begin() + static_cast<long>(decoded->bytesConsumed));
            writeResponse(std::move(response));
        }
    }

    void writeResponse(std::vector<std::byte> response) {
        auto self = shared_from_this();
        auto buf = std::make_shared<std::vector<std::byte>>(std::move(response));
        boost::asio::async_write(socket_, boost::asio::buffer(*buf),
                                 [self, buf](boost::system::error_code ec, std::size_t) {
                                     if (ec) {
                                         self->logger_.info("Write failed: " + ec.message());
                                     }
                                 });
    }

    boost::asio::ip::tcp::socket socket_;
    MessageDispatcher dispatcher_;
    common::Logger& logger_;
    std::array<char, 4096> readChunk_{};
    std::vector<std::byte> inbox_; // accumulates bytes across reads until a full frame exists
};

} // namespace exchange::gateway