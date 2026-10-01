#pragma once

#include "common/Logger.hpp"
#include "common/protocol/Serialization.hpp"
#include "persistence/ITradeRepository.hpp"

#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include <stdexcept>

namespace exchange::persistence {

// Consumes trades from Kafka and hands each to an ITradeRepository.
// Manual offset commit (enable.auto.commit=false) is the crux of the
// durability story: the offset only advances AFTER repository.save()
// returns successfully, so a crash between receiving a message and
// finishing the DB write results in that message being redelivered on
// restart - at-least-once, not "lost." Redelivery-safety then depends
// on the repository's insert being idempotent (see
// PostgresTradeRepository's ON CONFLICT DO NOTHING
class KafkaTradeConsumer {
public:
    KafkaTradeConsumer(const std::string& brokers, const std::string& topic,
                       const std::string& groupId, common::Logger& logger)
        : logger_(logger) {
        std::string errStr;
        std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
        conf->set("bootstrap.servers", brokers, errStr);
        conf->set("group.id", groupId, errStr);
        conf->set("enable.auto.commit", "false", errStr);
        // "earliest": a brand-new consumer group (e.g. first deployment,
        // or a rebuilt Persistence instance) replays the FULL trade
        // history from the start of the topic's retention window,
        // rather than only seeing trades from the moment it connects -
        // this is the actual "replay" guarantee this milestone exists
        // to provide, and is exactly what ZeroMQ PUB/SUB cannot do.
        conf->set("auto.offset.reset", "earliest", errStr);

        consumer_.reset(RdKafka::KafkaConsumer::create(conf.get(), errStr));
        if (!consumer_)
            throw std::runtime_error("Failed to create Kafka consumer: " + errStr);

        RdKafka::ErrorCode err = consumer_->subscribe({topic});
        if (err != RdKafka::ERR_NO_ERROR) {
            throw std::runtime_error("Failed to subscribe: " + RdKafka::err2str(err));
        }
    }

    // Blocks until one message is available (or timeoutMs elapses,
    // returning without doing anything - lets the caller's loop check
    // a shutdown flag periodically, same pattern as TradeTicker's
    // rcvtimeo in the client, Milestone 10).
    void pollOnce(int timeoutMs, ITradeRepository& repository) {
        std::unique_ptr<RdKafka::Message> msg(consumer_->consume(timeoutMs));

        if (msg->err() == RdKafka::ERR__TIMED_OUT)
            return;
        if (msg->err() != RdKafka::ERR_NO_ERROR) {
            logger_.warn("Kafka consume error: " + msg->errstr());
            return;
        }

        std::vector<std::byte> payload(static_cast<const std::byte*>(msg->payload()),
                                       static_cast<const std::byte*>(msg->payload()) + msg->len());

        common::protocol::ByteReader reader(payload);
        common::Trade trade = common::protocol::readTrade(reader);

        repository.save(trade); // idempotent - see class comment above

        // Only commit AFTER save() succeeds. If save() throws, we
        // deliberately do NOT commit - this message will be redelivered
        // on the next pollOnce() after a restart, which is exactly the
        // "never silently lose a trade" guarantee this milestone adds.
        consumer_->commitSync(msg.get());
    }

private:
    common::Logger& logger_;
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
};

} // namespace exchange::persistence