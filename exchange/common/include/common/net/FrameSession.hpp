#pragma once

#include "common/Logger.hpp"
#include "common/protocol/Frame.hpp"

#include <array>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace exchange::common::net {

using FrameHandler = std::function<std::vector<std::byte>(const protocol::DecodedFrame&)>;

// Generic per-connection session: accumulates bytes, decodes complete
// frames, invokes a caller-supplied handler, writes back the response.
// Extracted from the Gateway once Risk and Matching
// Engine needed the identical accept/read/dispatch loop - only *what a
// message means* differs between services, never *how bytes become a
// message*, so the latter lives here exactly once.
class FrameSession : public std::enable_shared_from_this<FrameSession> {
public:
    FrameSession(boost::asio::ip::tcp::socket socket, FrameHandler handler, common::Logger& logger)
        : socket_(std::move(socket)), handler_(std::move(handler)), logger_(logger) {}

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
                    logger_.info("Connection closed: " + ec.message());
                    return;
                }
                inbox_.insert(inbox_.end(), reinterpret_cast<std::byte*>(readChunk_.data()),
                              reinterpret_cast<std::byte*>(readChunk_.data()) + bytesRead);
                processCompleteFrames();
                readMore();
            });
    }

    void processCompleteFrames() {
        while (auto decoded = protocol::tryDecodeFrame(inbox_)) {
            auto response = handler_(*decoded);
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
                                     if (ec)
                                         self->logger_.info("Write failed: " + ec.message());
                                 });
    }

    boost::asio::ip::tcp::socket socket_;
    FrameHandler handler_;
    common::Logger& logger_;
    std::array<char, 4096> readChunk_{};
    std::vector<std::byte> inbox_;
};

} // namespace exchange::common::net