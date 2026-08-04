#pragma once

#include "common/protocol/ByteWriter.hpp"
#include "common/protocol/MessageType.hpp"

#include <optional>
#include <vector>

namespace exchange::common::protocol {

// Wraps a payload with the [length][type] framing header described in
// design notes. This is the only function that knows
// about the wire's framing format - callers just provide a message type
// and an already-serialized payload.
inline std::vector<std::byte> encodeFrame(MessageType type, const std::vector<std::byte>& payload) {
    ByteWriter header;
    // Length covers the type byte + payload, so the reader knows exactly
    // how many more bytes to wait for after the 4-byte length prefix.
    header.writeUInt32(static_cast<std::uint32_t>(payload.size() + 1));
    header.writeUInt8(static_cast<std::uint8_t>(type));

    std::vector<std::byte> frame = header.bytes();
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

struct DecodedFrame {
    MessageType type;
    std::vector<std::byte> payload;
    std::size_t bytesConsumed; // how much of the input buffer this frame used
};

// Attempts to decode one complete frame from the front of `buffer`.
// Returns std::nullopt if `buffer` does not yet contain a full frame -
// this is the normal, expected case when reading from a TCP stream,
// where a single recv() can return a partial message, keep buffering incoming
// bytes and retry this call until it succeeds.
inline std::optional<DecodedFrame> tryDecodeFrame(const std::vector<std::byte>& buffer) {
    constexpr std::size_t lengthPrefixSize = 4;
    if (buffer.size() < lengthPrefixSize) {
        return std::nullopt; // not even the length prefix has arrived yet
    }

    const std::uint32_t bodyLength = (std::to_integer<std::uint32_t>(buffer[0]) << 24) |
                                     (std::to_integer<std::uint32_t>(buffer[1]) << 16) |
                                     (std::to_integer<std::uint32_t>(buffer[2]) << 8) |
                                     (std::to_integer<std::uint32_t>(buffer[3]));

    const std::size_t totalFrameSize = lengthPrefixSize + bodyLength;
    if (buffer.size() < totalFrameSize) {
        return std::nullopt; // header arrived, but body is still incomplete
    }

    const auto type = static_cast<MessageType>(std::to_integer<std::uint8_t>(buffer[4]));
    std::vector<std::byte> payload(buffer.begin() + 5,
                                   buffer.begin() + static_cast<long>(totalFrameSize));

    return DecodedFrame{type, std::move(payload), totalFrameSize};


}

} // namespace exchange::common::protocol