#include <gtest/gtest.h>

#include "client/CommandParser.hpp"

using namespace exchange::client;
using namespace exchange::common;

TEST(CommandParserTest, ParsesBuyCommand) {
    auto result = CommandParser::parse("BUY AAPL 100 25");
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    auto cmd = std::get<Command>(result);
    ASSERT_TRUE(std::holds_alternative<BuyCommand>(cmd));
    auto buy = std::get<BuyCommand>(cmd);
    EXPECT_EQ(buy.symbol, "AAPL");
    EXPECT_EQ(buy.price, Price(100));
    EXPECT_EQ(buy.quantity, Quantity(25));
}

TEST(CommandParserTest, ParsesSellCommandCaseInsensitively) {
    auto result = CommandParser::parse("sell msft 200 10");
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    auto cmd = std::get<Command>(result);
    ASSERT_TRUE(std::holds_alternative<SellCommand>(cmd));
}

TEST(CommandParserTest, ParsesCancelCommand) {
    auto result = CommandParser::parse("CANCEL 42");
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    auto cmd = std::get<Command>(result);
    ASSERT_TRUE(std::holds_alternative<CancelCommand>(cmd));
    EXPECT_EQ(std::get<CancelCommand>(cmd).orderId, OrderId(42));
}

TEST(CommandParserTest, ParsesModifyCommand) {
    auto result = CommandParser::parse("MODIFY 7 150 30");
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    auto modify = std::get<ModifyCommand>(std::get<Command>(result));
    EXPECT_EQ(modify.orderId, OrderId(7));
    EXPECT_EQ(modify.price, Price(150));
    EXPECT_EQ(modify.quantity, Quantity(30));
}

TEST(CommandParserTest, ParsesBookCommand) {
    auto result = CommandParser::parse("BOOK AAPL");
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_EQ(std::get<BookCommand>(std::get<Command>(result)).symbol, "AAPL");
}

TEST(CommandParserTest, ParsesTradesAndQuit) {
    EXPECT_TRUE(
        std::holds_alternative<TradesCommand>(std::get<Command>(CommandParser::parse("TRADES"))));
    EXPECT_TRUE(
        std::holds_alternative<QuitCommand>(std::get<Command>(CommandParser::parse("QUIT"))));
    EXPECT_TRUE(
        std::holds_alternative<QuitCommand>(std::get<Command>(CommandParser::parse("exit"))));
}

TEST(CommandParserTest, EmptyLineReturnsHelp) {
    auto result = CommandParser::parse("");
    ASSERT_TRUE(std::holds_alternative<Command>(result));
    EXPECT_TRUE(std::holds_alternative<HelpCommand>(std::get<Command>(result)));
}

TEST(CommandParserTest, MissingArgumentsReturnsUsageError) {
    auto result = CommandParser::parse("BUY AAPL 100"); // missing qty
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
}

TEST(CommandParserTest, UnknownVerbReturnsError) {
    auto result = CommandParser::parse("FROB AAPL");
    ASSERT_TRUE(std::holds_alternative<std::string>(result));
}