#pragma once

#include "common/Types.hpp"

#include <optional>
#include <sstream>
#include <string>
#include <variant>

namespace exchange::client {

struct BuyCommand {
    common::Symbol symbol;
    common::Price price;
    common::Quantity quantity;
};
struct SellCommand {
    common::Symbol symbol;
    common::Price price;
    common::Quantity quantity;
};
struct CancelCommand {
    common::OrderId orderId;
};
struct ModifyCommand {
    common::OrderId orderId;
    common::Price price;
    common::Quantity quantity;
};
struct BookCommand {
    common::Symbol symbol;
};
struct TradesCommand {};
struct QuitCommand {};
struct HelpCommand {};

using Command = std::variant<BuyCommand, SellCommand, CancelCommand, ModifyCommand, BookCommand,
                             TradesCommand, QuitCommand, HelpCommand>;

// Parses one line of console input into a Command, or an error string
// if the line doesn't match any known command grammar. Kept entirely
// free of I/O and networking - this is pure text-parsing logic,
// unit-testable with plain strings and no socket/console involved
class CommandParser {
public:
    [[nodiscard]] static std::variant<Command, std::string> parse(const std::string& line) {
        std::istringstream iss(line);
        std::string verb;
        iss >> verb;
        for (auto& c : verb)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        if (verb == "BUY" || verb == "SELL") {
            std::string symbol;
            std::int64_t price = 0, qty = 0;
            if (!(iss >> symbol >> price >> qty)) {
                return std::string("usage: ") + verb + " <SYMBOL> <PRICE> <QTY>";
            }
            if (verb == "BUY") {
                return Command{BuyCommand{symbol, common::Price(price), common::Quantity(qty)}};
            }
            return Command{SellCommand{symbol, common::Price(price), common::Quantity(qty)}};
        }

        if (verb == "CANCEL") {
            std::uint64_t id = 0;
            if (!(iss >> id))
                return std::string("usage: CANCEL <ORDER_ID>");
            return Command{CancelCommand{common::OrderId(id)}};
        }

        if (verb == "MODIFY") {
            std::uint64_t id = 0;
            std::int64_t price = 0, qty = 0;
            if (!(iss >> id >> price >> qty)) {
                return std::string("usage: MODIFY <ORDER_ID> <PRICE> <QTY>");
            }
            return Command{
                ModifyCommand{common::OrderId(id), common::Price(price), common::Quantity(qty)}};
        }

        if (verb == "BOOK") {
            std::string symbol;
            if (!(iss >> symbol))
                return std::string("usage: BOOK <SYMBOL>");
            return Command{BookCommand{symbol}};
        }

        if (verb == "TRADES")
            return Command{TradesCommand{}};
        if (verb == "QUIT" || verb == "EXIT")
            return Command{QuitCommand{}};
        if (verb == "HELP" || verb.empty())
            return Command{HelpCommand{}};

        return std::string("unknown command: ") + verb;
    }
};

} // namespace exchange::client