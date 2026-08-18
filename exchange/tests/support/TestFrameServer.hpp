#pragma once

#include "common/Logger.hpp"
#include "common/net/FrameServer.hpp"

#include <thread>

namespace exchange::test_support {

// Test-only helper: runs a common::net::FrameServer on a background
// thread. Lets integration tests exercise a real socket end-to-end
// without Docker or launching separate processes.
class TestFrameServer {
public:
    TestFrameServer(unsigned short port, common::net::FrameHandler handler, std::string loggerName)
        : logger_(std::move(loggerName)), server_(io_, port, std::move(handler), logger_),
          thread_([this] { io_.run(); }) {}

    ~TestFrameServer() {
        io_.stop();
        thread_.join();
    }

private:
    boost::asio::io_context io_;
    common::Logger logger_;
    common::net::FrameServer server_;
    std::thread thread_;
};

} // namespace exchange::test_support