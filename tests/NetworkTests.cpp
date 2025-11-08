#include "network/tcpServer.h"
#include "network/tcpConnection.h"
#include "client/Client.h"
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <iostream>

// ===================================================
// Simple Network Tests
// ===================================================

// helper function to start a basic TcpServer on a background thread.
void startServer(unsigned short port, std::thread& serverThread);

// ===================================================
// Account creation and login
// ===================================================

void testCreateAccountRequest();

void testLoginRequest();

// ===================================================
// Main Entry
// ===================================================

int main() {
    std::cout << "=============================\n";
    std::cout << " Running Network Unit Tests\n";
    std::cout << "=============================\n";

    unsigned short port = 5555;
    std::thread serverThread;
    startServer(port, serverThread);

    // temporary sleep until async fully works
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    testCreateAccountRequest();
    testLoginRequest();

    std::cout << "\nAll tests executed.\n";

    if (serverThread.joinable())
        serverThread.detach();

    return 0;
}