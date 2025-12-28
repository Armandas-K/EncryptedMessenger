#include <asio.hpp>
#include <iostream>
#include "Network/TcpServer.h"

int main() {
    try {
        asio::io_context io;
        TcpServer server(io, 5555);
        io.run();
    }
    catch (const std::exception& e) {
        std::cerr << "[Server] error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}