#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace exchange::common {

    // Strong-typed wrapper to prevent unit-confusion bugs (e.g. passing a
    // Quantity where a Price is expected). Zero runtime overhead - this
    // compiles down to a plain integer.
    template <typename Tag, typename Underlying>
    class StrongType {
    public:
        constexpr StrongType() noexcept = default;
        constexpr explicit StrongType(Underlying value) noexcept : value_(value) {}

        [[nodiscard]] constexpr Underlying get() const noexcept { return value_; }

        constexpr auto operator<=>(const StrongType&) const = default;

    private:
        Underlying value_{};
    };

    struct PriceTag {};
    struct QuantityTag {};
    struct OrderIdTag {};

    // Price expressed in integer ticks (e.g. cents), never floating point.
    // Rationale: floating point prices cause rounding/comparison bugs in
    // matching logic. Real exchanges always use fixed-point/integer pricing.
    using Price = StrongType<PriceTag, std::int64_t>;

    // Order quantity in whole shares/units.
    using Quantity = StrongType<QuantityTag, std::int64_t>;

    // Unique, monotonically increasing order identifier assigned by the
    // matching engine.
    using OrderId = StrongType<OrderIdTag, std::uint64_t>;

    using Symbol = std::string;

    enum class Side : std::uint8_t {
        Buy,
        Sell
    };

    enum class OrderType : std::uint8_t {
        Limit
        // Market, Stop, Iceberg reserved for future milestones
    };

    using Timestamp = std::chrono::system_clock::time_point;

} // namespace exchange::common