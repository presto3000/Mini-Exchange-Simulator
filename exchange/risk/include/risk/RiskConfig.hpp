#pragma once

#include "common/Types.hpp"

namespace exchange::risk {

// Configurable risk limits. Kept as plain data (no behavior) so it can
// eventually be loaded from a config file or environment variables
// without touching RiskChecker's logic at all - the limits are data,
// the rules that use them are code, and mixing the two would make the
// limits harder to change independently of the checking logic.
struct RiskConfig {
    common::Quantity maxOrderQuantity;
};

} // namespace exchange::risk