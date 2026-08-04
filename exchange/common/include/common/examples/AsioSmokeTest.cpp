// Proves Boost.Asio is correctly found, configured, and linkable before
// builds real networking code on top of it. Deliberately
// tiny and disposable - not part of any service, just a build-verification
// step.
#include <boost/asio.hpp>
#include <iostream>

int main() {
    boost::asio::io_context io;
    std::cout << "Boost.Asio io_context constructed successfully.\n";
    return 0;
}