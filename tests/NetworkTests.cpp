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

// helper function to start a basic tcpServer on a background thread.
void startServer(unsigned short port, std::thread& serverThread) {
    serverThread = std::thread([port]() {
        try {
            asio::io_context io_context;
            TcpServer server(io_context, port);
            io_context.run();
        } catch (std::exception& e) {
            std::cerr << "[Test] Server exception: " << e.what() << std::endl;
        }
    });
}

// ===================================================
// Account creation and login
// ===================================================

void testCreateAccountRequest() {
    std::cout << "\n[Test] Running testCreateAccountRequest..." << std::endl;

    asio::io_context io_context;
    auto connection = TcpConnection::create(io_context, nullptr);
    Client client(connection);

    std::string username = "test_user";
    std::string password = "secure_password";

    bool result = client.createAccount(username, password);

    assert(result && "[testCreateAccountRequest] Client failed to send create account request.");
    std::cout << "[Test] CreateAccountRequest passed." << std::endl;
}

void testLoginRequest() {
    std::cout << "\n[Test] Running testLoginRequest..." << std::endl;

    asio::io_context io_context;
    auto connection = TcpConnection::create(io_context, nullptr);
    Client client(connection);

    std::string username = "test_user";
    std::string password = "secure_password";

    bool result = client.login(username, password);

    assert(result && "[testLoginRequest] Client failed to send login request.");
    std::cout << "[Test] LoginRequest passed." << std::endl;
}

// ===================================================
// Placeholders for future tests
// ===================================================

void testSendMessageRequest() {
    std::cout << "\n[Test] Placeholder: testSendMessageRequest" << std::endl;
    // todo: simulate sending message through TcpConnection
}

void testReceiveMessageResponse() {
    std::cout << "\n[Test] Placeholder: testReceiveMessageResponse" << std::endl;
    // todo: simulate receiving message from TcpServer
}

void testHandleDisconnectedClient() {
    std::cout << "\n[Test] Placeholder: testHandleDisconnectedClient" << std::endl;
    // todo: test proper cleanup of TcpConnection on disconnect
}

void testMultipleClientsSimultaneousConnections() {
    std::cout << "\n[Test] Placeholder: testMultipleClientsSimultaneousConnections" << std::endl;
    // todo: spawn several clients and verify server handles them
}

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
    testSendMessageRequest();
    testReceiveMessageResponse();
    testHandleDisconnectedClient();
    testMultipleClientsSimultaneousConnections();

    std::cout << "\nAll tests executed.\n";

    if (serverThread.joinable())
        serverThread.detach();

    return 0;
}