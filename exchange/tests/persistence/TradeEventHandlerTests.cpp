#include <gtest/gtest.h>

#include "persistence/TradeEventHandler.hpp"

using namespace exchange::common;
using namespace exchange::common::protocol;
using namespace exchange::persistence;

namespace {

// Test double: records every trade handed to it instead of writing to
// a real database. This is exactly why ITradeRepository exists as an
// interface - PostgresTradeRepository requires a live Postgres
// instance and is deliberately NOT exercised by this fast unit test;
// verifying it works is a docker-compose-level manual/integration
// check instead
class FakeTradeRepository : public ITradeRepository {
public:
    void save(const Trade& trade) override {
        savedTrades.push_back(trade);
    }
    std::vector<Trade> savedTrades;
};

} // namespace

TEST(PersistenceTradeEventHandlerTest, DecodedTradeIsSavedToRepository) {
    FakeTradeRepository repo;
    TradeEventHandler handler(repo);

    Trade trade(TradeId(7), OrderId(1), OrderId(2), "AAPL", Price(15000), Quantity(50), Timestamp{});
    ByteWriter w;
    writeTrade(w, trade);

    handler.handleTradePayload(w.bytes());

    ASSERT_EQ(repo.savedTrades.size(), 1u);
    EXPECT_EQ(repo.savedTrades[0].symbol(), "AAPL");
    EXPECT_EQ(repo.savedTrades[0].price(), Price(15000));
}

TEST(PersistenceTradeEventHandlerTest, MultipleTradesAreAllSaved) {
    FakeTradeRepository repo;
    TradeEventHandler handler(repo);

    for (int i = 0; i < 3; ++i) {
        Trade trade(TradeId(7), OrderId(i), OrderId(i + 10), "MSFT", Price(30000 + i), Quantity(5),
                    Timestamp{});
        ByteWriter w;
        writeTrade(w, trade);
        handler.handleTradePayload(w.bytes());
    }

    EXPECT_EQ(repo.savedTrades.size(), 3u);
}

TEST(PersistenceTradeEventHandlerTest, DuplicateTradeIdIsSavedButRepositoryDecidesDeduping) {
    // The handler layer itself doesn't dedupe - it hands every trade it
    // decodes to the repository, exactly as before. Deduping is a
    // PostgresTradeRepository/database concern (ON CONFLICT DO NOTHING),
    // which real Postgres integration testing - not this fast unit test
    // - is the right place to verify. This test documents that boundary
    // explicitly rather than pretending the fake repository re-implements
    // Postgres's constraint behavior.
    FakeTradeRepository repo;
    TradeEventHandler handler(repo);

    Trade trade(TradeId(5), OrderId(1), OrderId(2), "AAPL", Price(100), Quantity(10), Timestamp{});
    ByteWriter w;
    writeTrade(w, trade);

    handler.handleTradePayload(w.bytes());
    handler.handleTradePayload(w.bytes()); // simulates Kafka redelivery

    EXPECT_EQ(repo.savedTrades.size(), 2u); // handler passes both through by design
    EXPECT_EQ(repo.savedTrades[0].id(),
              repo.savedTrades[1].id()); // same TradeId - real DB would collapse these
}