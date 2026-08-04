#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace exchange::common::protocol {

// Reads fixed-width integers and length-prefixed strings back out of a
// byte buffer, in the same order ByteWriter wrote them. Throws
// std::out_of_range on a truncated buffer rather than reading past the
// end - a malformed/partial frame must never cause undefined behavior,
// since this code parses data arriving from another process over the
// network.
class ByteReader {
public:
    explicit ByteReader(const std::vector<std::byte>& data) : data_(data) {}

    std::uint8_t readUInt8() {
        checkAvailable(1);
        return std::to_integer<std::uint8_t>(data_[pos_++]);
    }

    std::uint32_t readUInt32() {
        checkAvailable(4);
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            v = (v << 8) | std::to_integer<std::uint32_t>(data_[pos_++]);
        }
        return v;
    }

    std::int64_t readInt64() {
        return static_cast<std::int64_t>(readUInt64());
    }

    std::uint64_t readUInt64() {
        checkAvailable(8);
        std::uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v = (v << 8) | std::to_integer<std::uint64_t>(data_[pos_++]);
        }
        return v;
    }

    std::string readString() {
        const auto len = readUInt32();
        checkAvailable(len);
        std::string s;
        s.reserve(len);
        for (std::uint32_t i = 0; i < len; ++i) {
            s.push_back(static_cast<char>(std::to_integer<unsigned char>(data_[pos_++])));
        }
        return s;
    }

private:
    void checkAvailable(std::size_t n) const {
        if (pos_ + n > data_.size()) {
            throw std::out_of_range("ByteReader: attempted to read past end of buffer");
        }
    }

    const std::vector<std::byte>& data_;
    std::size_t pos_ = 0;
};

} // namespace exchange::common::protocol