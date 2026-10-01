#include "common/Logger.hpp"
#include "persistence/KafkaTradeConsumer.hpp"
#include "persistence/PostgresTradeRepository.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

namespace {
std::string getEnvOr(const char* name, std::string fallback) {
    if (const char* v = std::getenv(name))
        return std::string(v);
    return fallback;
}
} // namespace

int main() {
    using namespace exchange;
    common::Logger logger("persistence_service");
    logger.info("Persistence service starting up.");

    const std::string connStr = "host=" + getEnvOr("POSTGRES_HOST", "localhost") +
                                " port=" + getEnvOr("POSTGRES_PORT", "5432") +
                                " dbname=" + getEnvOr("POSTGRES_DB", "exchange") +
                                " user=" + getEnvOr("POSTGRES_USER", "exchange") +
                                " password=" + getEnvOr("POSTGRES_PASSWORD", "exchange");

    std::unique_ptr<persistence::PostgresTradeRepository> repository;
    for (int attempt = 1; attempt <= 10 && !repository; ++attempt) {
        try {
            repository = std::make_unique<persistence::PostgresTradeRepository>(connStr);
        } catch (const std::exception& e) {
            logger.warn("Postgres not ready (attempt " + std::to_string(attempt) +
                        "): " + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    if (!repository) {
        logger.error("Failed to connect to Postgres after retries. Exiting.");
        return 1;
    }
    logger.info("Connected to Postgres.");

    const std::string kafkaBrokers = getEnvOr("KAFKA_BROKERS", "localhost:9092");
    std::unique_ptr<persistence::KafkaTradeConsumer> consumer;
    for (int attempt = 1; attempt <= 10 && !consumer; ++attempt) {
        try {
            consumer = std::make_unique<persistence::KafkaTradeConsumer>(
                kafkaBrokers, "trades", "persistence-service", logger);
        } catch (const std::exception& e) {
            logger.warn("Kafka not ready (attempt " + std::to_string(attempt) + "): " + e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    if (!consumer) {
        logger.error("Failed to connect to Kafka after retries. Exiting.");
        return 1;
    }
    logger.info("Consuming from Kafka topic 'trades', group 'persistence-service'.");

    while (true) {
        consumer->pollOnce(1000, *repository);
    }
}