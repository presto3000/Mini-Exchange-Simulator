#include "client/CommandParser.hpp"
#include "client/ExchangeClient.hpp"
#include "client/TradeTicker.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
std::string getEnvOr(const char* name, std::string fallback) {
    if (const char* v = std::getenv(name))
        return std::string(v);
    return fallback;
}

void printHelp() {
    std::cout << "Commands:\n"
              << "  BUY <SYMBOL> <PRICE> <QTY>\n"
              << "  SELL <SYMBOL> <PRICE> <QTY>\n"
              << "  CANCEL <ORDER_ID>\n"
              << "  MODIFY <ORDER_ID> <PRICE> <QTY>\n"
              << "  BOOK <SYMBOL>\n"
              << "  TRADES        (streams live trades until QUIT)\n"
              << "  QUIT\n";
}

void printAckOrReject(const exchange::common::protocol::DecodedFrame& response) {
    using namespace exchange::common::protocol;
    if (response.type == MessageType::OrderAck) {
        ByteReader reader(response.payload);
        auto ack = readOrderAck(reader);
        std::cout << "ACK order_id=" << ack.orderId.get();
        if (ack.trades.empty()) {
            std::cout << " (resting, no trade yet)\n";
        } else {
            std::cout << " trades:\n";
            for (const auto& t : ack.trades) {
                std::cout << "    price=" << t.price().get() << " qty=" << t.quantity().get()
                          << " buy_id=" << t.buyOrderId().get()
                          << " sell_id=" << t.sellOrderId().get() << "\n";
            }
        }
    } else if (response.type == MessageType::OrderReject) {
        ByteReader reader(response.payload);
        auto reject = readOrderReject(reader);
        std::cout << "REJECTED order_id=" << reject.orderId.get() << " reason=" << reject.reason
                  << "\n";
    } else {
        std::cout << "unexpected response type from server\n";
    }
}

void printBook(const exchange::common::protocol::DecodedFrame& response) {
    using namespace exchange::common::protocol;
    if (response.type != MessageType::BookSnapshot) {
        std::cout << "unexpected response to BOOK query\n";
        return;
    }
    ByteReader reader(response.payload);
    auto snap = readBookSnapshot(reader);

    std::cout << "Book: " << snap.symbol << "\n";
    std::cout << "  BIDS:\n";
    for (const auto& lvl : snap.bids) {
        std::cout << "    " << lvl.price.get() << "  qty=" << lvl.totalQuantity.get()
                  << "  orders=" << lvl.orderCount << "\n";
    }
    std::cout << "  ASKS:\n";
    for (const auto& lvl : snap.asks) {
        std::cout << "    " << lvl.price.get() << "  qty=" << lvl.totalQuantity.get()
                  << "  orders=" << lvl.orderCount << "\n";
    }
}
} // namespace

int main() {
    using namespace exchange;
    using namespace exchange::client;

    const std::string gatewayHost = getEnvOr("GATEWAY_HOST", "localhost");
    const unsigned short gatewayPort =
        static_cast<unsigned short>(std::stoi(getEnvOr("GATEWAY_PORT", "9000")));
    const std::string marketDataHost = getEnvOr("MARKET_DATA_HOST", "localhost");
    const unsigned short marketDataPort =
        static_cast<unsigned short>(std::stoi(getEnvOr("MARKET_DATA_PORT", "9600")));

    std::cout << "Mini Exchange Simulator - console client\n";
    std::cout << "Connecting to gateway at " << gatewayHost << ":" << gatewayPort << " ...\n";

    ExchangeClient exchangeClient(gatewayHost, gatewayPort);
    zmq::context_t zmqContext(1);
    TradeTicker ticker(zmqContext, marketDataHost, marketDataPort);

    std::cout << "Connected. Type HELP for commands.\n";

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line))
            break;

        auto parsed = CommandParser::parse(line);
        if (std::holds_alternative<std::string>(parsed)) {
            std::cout << std::get<std::string>(parsed) << "\n";
            continue;
        }

        auto command = std::get<Command>(parsed);
        bool shouldQuit = false;

        std::visit(
            [&](auto&& cmd) {
                using T = std::decay_t<decltype(cmd)>;

                if constexpr (std::is_same_v<T, BuyCommand> || std::is_same_v<T, SellCommand>) {
                    constexpr auto side =
                        std::is_same_v<T, BuyCommand> ? common::Side::Buy : common::Side::Sell;
                    common::Order order(exchangeClient.nextOrderId(), cmd.symbol, side, cmd.price,
                                        cmd.quantity, std::chrono::system_clock::now());
                    printAckOrReject(exchangeClient.submitOrder(order));
                } else if constexpr (std::is_same_v<T, CancelCommand>) {
                    printAckOrReject(exchangeClient.cancelOrder(cmd.orderId));
                } else if constexpr (std::is_same_v<T, ModifyCommand>) {
                    printAckOrReject(
                        exchangeClient.modifyOrder(cmd.orderId, cmd.price, cmd.quantity));
                } else if constexpr (std::is_same_v<T, BookCommand>) {
                    printBook(exchangeClient.queryBook(cmd.symbol));
                } else if constexpr (std::is_same_v<T, TradesCommand>) {
                    std::cout << "Streaming trades (this blocks further commands until QUIT)...\n";
                    ticker.start();
                    std::string stopLine;
                    while (std::getline(std::cin, stopLine)) {
                        if (CommandParser::parse(stopLine).index() == 0 &&
                            std::holds_alternative<QuitCommand>(
                                std::get<Command>(CommandParser::parse(stopLine)))) {
                            break;
                        }
                    }
                    ticker.stop();
                } else if constexpr (std::is_same_v<T, QuitCommand>) {
                    shouldQuit = true;
                } else if constexpr (std::is_same_v<T, HelpCommand>) {
                    printHelp();
                }
            },
            command);

        if (shouldQuit)
            break;
    }

    std::cout << "Goodbye.\n";
    return 0;
}