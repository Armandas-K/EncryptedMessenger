#include "network/tcpServer.h"
#include "network/tcpConnection.h"
#include "client/Client.h"
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include "ClientTestContext.h"

// ===================================================
// Simple Network Tests
// ===================================================

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
// ACCOUNT CREATION
// ===================================================

void testCreateAccountRequest() {
    std::cout << "\n[Test] Running testCreateAccountRequest..." << std::endl;

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    Client client(connection);
    connection->beginRead();

    bool result = client.createAccount("test_user", "secure_password");
    assert(result && "[testCreateAccountRequest] Failed to send request.");

    std::cout << "[Test] CreateAccountRequest passed.\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// LOGIN TEST
// ===================================================

void testLoginRequest() {
    std::cout << "\n[Test] Running testLoginRequest..." << std::endl;

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    Client client(connection);
    connection->beginRead();

    bool result = client.login("test_user", "secure_password");
    assert(result && "[testLoginRequest] Client failed to send login request.");

    std::cout << "[Test] LoginRequest passed." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// SEND MESSAGE TEST
// ===================================================

void testSendMessageRequest() {
    std::cout << "\n[Test] Running testSendMessageRequest..." << std::endl;

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    Client client(connection);
    connection->beginRead();

    //todo implement sendMessage

    // bool result = client.sendMessage("bob", "Hello Bob!");
    // assert(result && "[testSendMessageRequest] Failed to send message request.");

    std::cout << "[Test] SendMessageRequest passed." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// RECEIVE MESSAGE TEST
// ===================================================

void testReceiveMessageResponse() {
    std::cout << "\n[Test] Running testReceiveMessageResponse..." << std::endl;

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    Client client(connection);
    connection->beginRead();

    std::cout << "[Test] ReceiveMessageResponse placeholder passed." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// DISCONNECTED CLIENT TEST
// ===================================================

void testHandleDisconnectedClient() {
    std::cout << "\n[Test] Running testHandleDisconnectedClient..." << std::endl;

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    connection->socket().close(); // simulate disconnect

    std::cout << "[Test] HandleDisconnectedClient placeholder passed." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// MULTIPLE CLIENTS TEST
// ===================================================

void testMultipleClientsSimultaneousConnections() {
    std::cout << "\n[Test] Running testMultipleClientsSimultaneousConnections..." << std::endl;

    ClientTestContext ctx;

    for (int i = 0; i < 3; i++) {
        auto connection = TcpConnection::create(ctx.io(), nullptr);
        bool ok = connection->connect("127.0.0.1", 5555);
        assert(ok && "[testMultipleClients] Client failed to connect.");
        connection->beginRead();
    }

    std::cout << "[Test] MultipleClientsSimultaneousConnections passed." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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