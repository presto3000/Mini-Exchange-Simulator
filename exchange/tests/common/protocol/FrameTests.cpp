#include <gtest/gtest.h>

#include "common/protocol/Frame.hpp"

using namespace exchange::common::protocol;

TEST(FrameTest, EncodeThenDecodeRecoversTypeAndPayload) {
    std::vector<std::byte> payload{std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}};
    auto frame = encodeFrame(MessageType::NewOrder, payload);

    auto decoded = tryDecodeFrame(frame);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->type, MessageType::NewOrder);
    EXPECT_EQ(decoded->payload, payload);
    EXPECT_EQ(decoded->bytesConsumed, frame.size());
}

TEST(FrameTest, IncompleteLengthPrefixReturnsNullopt) {
    std::vector<std::byte> buffer{std::byte{0}, std::byte{0}}; // only 2 of 4 bytes
    EXPECT_FALSE(tryDecodeFrame(buffer).has_value());
}

TEST(FrameTest, IncompleteBodyReturnsNullopt) {
    std::vector<std::byte> payload{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
                                   std::byte{5}};
    auto frame = encodeFrame(MessageType::TradeEvent, payload);

    // Truncate the frame - simulates a partial TCP read.
    frame.resize(frame.size() - 2);

    EXPECT_FALSE(tryDecodeFrame(frame).has_value());
}

TEST(FrameTest, MultipleFramesCanBeDecodedSequentially) {
    auto frame1 = encodeFrame(MessageType::NewOrder, {std::byte{1}});
    auto frame2 = encodeFrame(MessageType::OrderAck, {std::byte{2}, std::byte{3}});

    std::vector<std::byte> buffer = frame1;
    buffer.insert(buffer.end(), frame2.begin(), frame2.end());

    auto first = tryDecodeFrame(buffer);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->type, MessageType::NewOrder);

    // Simulate the read loop consuming the first frame and retrying on
    // the remainder - this is exactly what Asio loop does.
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<long>(first->bytesConsumed));

    auto second = tryDecodeFrame(buffer);
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->type, MessageType::OrderAck);
    EXPECT_EQ(second->payload.size(), 2u);
}