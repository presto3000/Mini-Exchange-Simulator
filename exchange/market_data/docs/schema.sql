CREATE TABLE IF NOT EXISTS trades (
    id            BIGSERIAL PRIMARY KEY,
    buy_order_id  BIGINT NOT NULL,
    sell_order_id BIGINT NOT NULL,
    symbol        TEXT   NOT NULL,
    price         BIGINT NOT NULL,
    quantity      BIGINT NOT NULL,
    trade_time    TIMESTAMPTZ NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_trades_symbol ON trades(symbol);