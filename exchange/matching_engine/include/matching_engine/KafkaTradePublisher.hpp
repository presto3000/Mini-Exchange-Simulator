#pragma once

#include "common/Logger.hpp"
#include "common/Trade.hpp"
#include "common/protocol/Serialization.hpp"

#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include <stdexcept>

namespace exchange::matching_engine {

// Publishes every Trade to a Kafka topic, keyed by symbol so all trades
// for one instrument land on the same partition (preserving per-symbol
// order for any consumer that cares - Persistence does, since it wants
// to write trades for a symbol in the order they occurred).
//
// This exists ALONGSIDE TradePublisher (ZeroMQ), not instead of it -
//  Market Data stays on ZeroMQ
// while Persistence moves to this durable, replayable feed instead.
class KafkaTradePublisher {
public:
    KafkaTradePublisher(const std::string& brokers, std::string topic, common::Logger& logger)
        : topic_(std::move(topic)), logger_(logger) {
        std::string errStr;
        std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
        conf->set("bootstrap.servers", brokers, errStr);
        // acks=all: don't consider a trade "published" until Kafka's
        // replicas (or the single broker, in our dev setup) have
        // actually persisted it - matches the durability guarantee this
        // whole milestone exists to provide.
        conf->set("acks", "all", errStr);

        producer_.reset(RdKafka::Producer::create(conf.get(), errStr));
        if (!producer_) {
            throw std::runtime_error("Failed to create Kafka producer: " + errStr);
        }
    }

    void publish(const common::Trade& trade) {
        common::protocol::ByteWriter writer;
        common::protocol::writeTrade(writer, trade);
        const auto& bytes = writer.bytes();

        RdKafka::ErrorCode err = producer_->produce(
            topic_,
            RdKafka::Topic::PARTITION_UA, // let the key determine the partition
            RdKafka::Producer::RK_MSG_COPY, const_cast<std::byte*>(bytes.data()), bytes.size(),
            trade.symbol().data(), trade.symbol().size(), 0, nullptr);

        if (err != RdKafka::ERR_NO_ERROR) {
            logger_.error("Kafka produce failed: " + std::string(RdKafka::err2str(err)));
        }
        producer_->poll(0); // drives delivery-report callbacks; non-blocking
    }

private:
    std::string topic_;
    common::Logger& logger_;
    std::unique_ptr<RdKafka::Producer> producer_;
};

} // namespace exchange::matching_engine