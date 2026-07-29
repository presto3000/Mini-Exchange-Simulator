#include "common/Logger.hpp"
#include "risk/RiskChecker.hpp"

int main() {
    using namespace exchange;

    common::Logger logger("risk_service");
    logger.info("Risk service starting up.");

    // Placeholder configuration - will be loaded from environment/config
    // file once the service is containerized and
    // driven by real Gateway traffic.
    risk::RiskConfig config{.maxOrderQuantity = common::Quantity(10'000)};
    risk::RiskChecker checker(config);

    logger.info("Risk checker initialized with maxOrderQuantity=10000.");
    logger.info("Network listener not yet implemented");

    // Intentionally does not exit immediately in a real service context;
    // for now, since there is no listener loop yet, we return cleanly to
    // keep this an honest, truthful stub rather than a fake infinite loop
    // that does nothing.

    return 0;
}