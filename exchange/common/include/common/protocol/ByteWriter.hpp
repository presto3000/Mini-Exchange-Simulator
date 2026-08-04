#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace exchange::common::protocol {

// Appends fixed-width integers and length-prefixed strings to a byte
// buffer in network (big-endian) order. Kept deliberately tiny - this
// is not a general-purpose serialization library, just enough to encode
// the handful of fields Order/Trade actually have.
class ByteWriter {
public:
    void writeUInt8(std::uint8_t v) {
        buffer_.push_back(std::byte{v});
    }

    void writeUInt32(std::uint32_t v) {
        buffer_.push_back(std::byte((v >> 24) & 0xFF));
        buffer_.push_back(std::byte((v >> 16) & 0xFF));
        buffer_.push_back(std::byte((v >> 8) & 0xFF));
        buffer_.push_back(std::byte(v & 0xFF));
    }

    void writeInt64(std::int64_t v) {
        writeUInt64(static_cast<std::uint64_t>(v));
    }

    void writeUInt64(std::uint64_t v) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            buffer_.push_back(std::byte((v >> shift) & 0xFF));
        }
    }

    void writeString(const std::string& s) {
        writeUInt32(static_cast<std::uint32_t>(s.size()));
        for (char c : s) {
            buffer_.push_back(std::byte(static_cast<unsigned char>(c)));
        }
    }

    [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept {
        return buffer_;
    }

private:
    std::vector<std::byte> buffer_;
};

} // namespace exchange::common::protocol