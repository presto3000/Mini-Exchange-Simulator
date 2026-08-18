#pragma once

#include "common/protocol/Frame.hpp"

#include <array>
#include <boost/asio.hpp>
#include <string>
#include <vector>

namespace exchange::common::net {

// A minimal blocking TCP client: connect once, then for each call write
// one framed request and block until one full framed response arrives.
//
// Deliberately synchronous even though the client-facing side of this
// system is async (see FrameSession). This client models internal
// service-to-service hops (Gateway->Risk, Risk->MatchingEngine), which
// are inherently one-request-waits-for-one-response chains. The real
// cost: a blocking call here stalls the calling service's single
// io_context thread for that round-trip, so no other client on that
// service is served concurrently during the wait. For this project's
// scale this is an acceptable, explicitly-named tradeoff; an async
// pipelined client (or a small connection pool) is the natural upgrade
// path and is listed as a future improvement.
class SyncTcpClient {
public:
    SyncTcpClient(std::string host, unsigned short port)
        : host_(std::move(host)), port_(port), socket_(io_) {}

    // Throws boost::system::system_error if the remote isn't reachable.
    // Callers treat that as "downstream service unavailable."
    void connect() {
        boost::asio::ip::tcp::resolver resolver(io_);
        auto endpoints = resolver.resolve(host_, std::to_string(port_));
        boost::asio::connect(socket_, endpoints);
    }

    protocol::DecodedFrame sendAndReceive(const std::vector<std::byte>& requestFrame) {
        boost::asio::write(socket_, boost::asio::buffer(requestFrame));

        while (true) {
            if (auto decoded = protocol::tryDecodeFrame(inbox_)) {
                inbox_.erase(inbox_.begin(),
                             inbox_.begin() + static_cast<long>(decoded->bytesConsumed));
                return *decoded;
            }
            std::array<char, 4096> chunk{};
            const std::size_t n = socket_.read_some(boost::asio::buffer(chunk));
            inbox_.insert(inbox_.end(), reinterpret_cast<std::byte*>(chunk.data()),
                          reinterpret_cast<std::byte*>(chunk.data()) + n);
        }
    }

private:
    std::string host_;
    unsigned short port_;
    boost::asio::io_context io_;
    boost::asio::ip::tcp::socket socket_;
    std::vector<std::byte> inbox_;
};

} // namespace exchange::common::net