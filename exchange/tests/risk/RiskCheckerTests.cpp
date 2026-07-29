#include <gtest/gtest.h>

#include "risk/RiskChecker.hpp"

using namespace exchange::common;
using namespace exchange::risk;

namespace {

Order makeOrder(Side side, Price price, Quantity qty) {
    return Order(OrderId(1), "AAPL", side, price, qty, Timestamp{});
}

RiskChecker makeChecker(std::int64_t maxQty = 1000) {
    return RiskChecker(RiskConfig{Quantity(maxQty)});
}

} // namespace

TEST(RiskCheckerTest, AcceptsValidBuyOrderWithSufficientBuyingPower) {
    RiskChecker checker = makeChecker();
    Account account("acct-1", BuyingPower(100'000));

    auto order = makeOrder(Side::Buy, Price(100), Quantity(50)); // cost = 5000
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::Accepted);
}

TEST(RiskCheckerTest, AcceptedBuyOrderReservesBuyingPower) {
    RiskChecker checker = makeChecker();
    Account account("acct-1", BuyingPower(100'000));

    auto order = makeOrder(Side::Buy, Price(100), Quantity(50)); // cost = 5000
    checker.checkOrder(order, account);

    EXPECT_EQ(account.buyingPower(), BuyingPower(95'000));
}

TEST(RiskCheckerTest, RejectsNonPositivePrice) {
    RiskChecker checker = makeChecker();
    Account account("acct-1", BuyingPower(100'000));

    auto order = makeOrder(Side::Buy, Price(0), Quantity(10));
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::RejectedNonPositivePrice);
}

TEST(RiskCheckerTest, RejectsNegativePrice) {
    RiskChecker checker = makeChecker();
    Account account("acct-1", BuyingPower(100'000));

    auto order = makeOrder(Side::Sell, Price(-10), Quantity(10));
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::RejectedNonPositivePrice);
}

TEST(RiskCheckerTest, RejectsQuantityAboveMax) {
    RiskChecker checker = makeChecker(/*maxQty=*/500);
    Account account("acct-1", BuyingPower(1'000'000));

    auto order = makeOrder(Side::Buy, Price(10), Quantity(501));
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::RejectedMaxQuantityExceeded);
}

TEST(RiskCheckerTest, AcceptsQuantityExactlyAtMax) {
    RiskChecker checker = makeChecker(/*maxQty=*/500);
    Account account("acct-1", BuyingPower(1'000'000));

    auto order = makeOrder(Side::Buy, Price(10), Quantity(500)); // == max, not >
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::Accepted);
}

TEST(RiskCheckerTest, RejectsInsufficientBuyingPower) {
    RiskChecker checker = makeChecker();
    Account account("acct-1", BuyingPower(1000)); // not enough

    auto order = makeOrder(Side::Buy, Price(100), Quantity(50)); // cost = 5000
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::RejectedInsufficientBuyingPower);
}

TEST(RiskCheckerTest, RejectedOrderDoesNotMutateAccount) {
    RiskChecker checker = makeChecker();
    Account account("acct-1", BuyingPower(1000));
    const auto before = account.buyingPower();

    auto order = makeOrder(Side::Buy, Price(100), Quantity(50)); // rejected
    checker.checkOrder(order, account);

    EXPECT_EQ(account.buyingPower(), before); // untouched on rejection
}

TEST(RiskCheckerTest, SellOrderDoesNotConsumeBuyingPower) {
    RiskChecker checker = makeChecker();
    Account account("acct-1", BuyingPower(1000)); // low, but SELL shouldn't care

    auto order = makeOrder(Side::Sell, Price(100), Quantity(50));
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::Accepted);
    EXPECT_EQ(account.buyingPower(), BuyingPower(1000)); // unchanged
}

TEST(RiskCheckerTest, MaxQuantityCheckedBeforeBuyingPowerCheck) {
    // An order that violates BOTH max-quantity and buying-power should
    // report the max-quantity rejection, since that check runs first -
    // verifies the short-circuit ordering is deterministic and documented.
    RiskChecker checker = makeChecker(/*maxQty=*/10);
    Account account("acct-1", BuyingPower(0));

    auto order = makeOrder(Side::Buy, Price(100), Quantity(9999));
    auto outcome = checker.checkOrder(order, account);

    EXPECT_EQ(outcome, RiskOutcome::RejectedMaxQuantityExceeded);
}