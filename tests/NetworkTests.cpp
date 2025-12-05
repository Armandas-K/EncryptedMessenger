#include "network/tcpServer.h"
#include "network/tcpConnection.h"
#include "client/Client.h"
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include "utils/ClientTestContext.h"
#include "utils/Logger.h"

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
    Logger::log("\n[Test] Running testCreateAccountRequest...");

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

    Logger::log("[Test] CreateAccountRequest passed.\n");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// LOGIN TEST
// ===================================================

void testLoginRequest() {
    Logger::log("\n[Test] Running testLoginRequest...");

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

    Logger::log("[Test] LoginRequest passed.");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// SEND MESSAGE TEST
// ===================================================

void testSendMessageRequest() {
    Logger::log("\n[Test] Running testSendMessageRequest...");

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    Client client(connection);
    connection->beginRead();

    assert(client.login("test_user", "secure_password")
           && "[testSendMessageRequest] Login failed");

    assert(client.sendMessage("bob", "Hello Bob!")
           && "[testSendMessageRequest] Failed to send message");

    Logger::log("[Test] SendMessageRequest passed.");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

// ===================================================
// RECEIVE MESSAGE TEST
// ===================================================

void testReceiveMessageResponse() {
    Logger::log("\n[Test] Running testReceiveMessageResponse...");

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    Client client(connection);
    connection->beginRead();

    Logger::log("[Test] ReceiveMessageResponse placeholder passed.");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// DISCONNECTED CLIENT TEST
// ===================================================

void testHandleDisconnectedClient() {
    Logger::log("\n[Test] Running testHandleDisconnectedClient...");

    ClientTestContext ctx;

    auto connection = TcpConnection::create(ctx.io(), nullptr);

    if (!connection->connect("127.0.0.1", 5555)) {
        std::cerr << "[Test] Client failed to connect.\n";
        return;
    }

    connection->socket().close(); // simulate disconnect

    Logger::log("[Test] HandleDisconnectedClient placeholder passed.");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// MULTIPLE CLIENTS TEST
// ===================================================

void testMultipleClientsSimultaneousConnections() {
    Logger::log("\n[Test] Running testMultipleClientsSimultaneousConnections...");

    ClientTestContext ctx;

    for (int i = 0; i < 3; i++) {
        auto connection = TcpConnection::create(ctx.io(), nullptr);
        bool ok = connection->connect("127.0.0.1", 5555);
        assert(ok && "[testMultipleClients] Client failed to connect.");
        connection->beginRead();
    }

    Logger::log("[Test] MultipleClientsSimultaneousConnections passed.");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// Main Entry
// ===================================================

int main() {
    Logger::log("=============================\n");
    Logger::log(" Running Network Unit Tests\n");
    Logger::log("=============================\n");

    unsigned short port = 5555;
    std::thread serverThread;
    startServer(port, serverThread);

    // temporary sleep until async fully works
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    testCreateAccountRequest();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    testLoginRequest();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    testSendMessageRequest();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    testReceiveMessageResponse();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    testHandleDisconnectedClient();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    testMultipleClientsSimultaneousConnections();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    Logger::log("\nAll tests executed.\n");

    if (serverThread.joinable())
        serverThread.detach();

    return 0;
}