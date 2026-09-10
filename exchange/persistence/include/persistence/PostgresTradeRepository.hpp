#pragma once

#include "persistence/ITradeRepository.hpp"

#include <pqxx/pqxx>

namespace exchange::persistence {

// The real, production concrete repository. The Matching Engine never
// talks to this, or to Postgres, directly - per the spec's explicit
// requirement, only Persistence (subscribing to the trade event feed)
// touches the database, keeping matching latency completely decoupled
// from storage latency.
class PostgresTradeRepository : public ITradeRepository {
public:
    explicit PostgresTradeRepository(const std::string& connectionString)
        : connection_(connectionString) {
        pqxx::work txn(connection_);
        txn.exec("CREATE TABLE IF NOT EXISTS trades ("
                 "  id BIGSERIAL PRIMARY KEY,"
                 "  buy_order_id BIGINT NOT NULL,"
                 "  sell_order_id BIGINT NOT NULL,"
                 "  symbol TEXT NOT NULL,"
                 "  price BIGINT NOT NULL,"
                 "  quantity BIGINT NOT NULL,"
                 "  trade_time TIMESTAMPTZ NOT NULL"
                 ")");
        txn.commit();
    }

    void save(const common::Trade& trade) override {
        pqxx::work txn(connection_);
        txn.exec_params(
            "INSERT INTO trades (buy_order_id, sell_order_id, symbol, price, quantity, trade_time) "
            "VALUES ($1, $2, $3, $4, $5, to_timestamp($6))",
            trade.buyOrderId().get(), trade.sellOrderId().get(), trade.symbol(),
            trade.price().get(), trade.quantity().get(),
            std::chrono::duration<double>(trade.timestamp().time_since_epoch()).count());
        txn.commit();
    }

private:
    pqxx::connection connection_;
};

} // namespace exchange::persistence