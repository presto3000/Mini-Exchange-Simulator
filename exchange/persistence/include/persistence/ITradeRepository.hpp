#pragma once

#include "common/Trade.hpp"

namespace exchange::persistence {

// Abstraction over trade storage. Same Dependency Inversion pattern as
// IOrderProcessor: the message-handling logic below
// depends on this interface, never on pqxx/Postgres directly, which is
// exactly what lets us unit-test that logic with a fake in-memory
// repository instead of requiring a real running PostgreSQL instance
// for every test run.
class ITradeRepository {
public:
    virtual ~ITradeRepository() = default;
    virtual void save(const common::Trade& trade) = 0;
};

} // namespace exchange::persistence