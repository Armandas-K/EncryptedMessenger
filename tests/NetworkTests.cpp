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
// Delete Old User Data
// ===================================================

// could reset entire file system with ofstream but that would require
// a directory for test user data separate from normal data
void resetUsers() {
    FileStorage storage = FileStorage();
    for (int i = 0; i < 10; i++) {
        storage.deleteUser("test_user_" + std::to_string(i));
    }
    storage.saveUser();
}

// ===================================================
// Set Up TcpServer
// ===================================================

void startServer(unsigned short port, std::thread& serverThread) {
    serverThread = std::thread([port]() {
        try {
            asio::io_context io;
            TcpServer server(io, port);
            io.run();
        } catch (std::exception& e) {
            std::cerr << "[Test] Server exception: " << e.what() << std::endl;
        }
    });
}

// ===================================================
// ACCOUNT CREATION
// ===================================================

int userCount = 0;
std::string makeUser() {
    return "test_user_" + std::to_string(++userCount);
}

// create 2 users
void testCreateAccountRequest() {
    Logger::log("\n[Test] Running testCreateAccountRequest...");

    ClientTestContext ctx;

    auto conn = TcpConnection::create(ctx.io(), nullptr);
    assert(conn->connect("127.0.0.1", 5555));

    Client client(conn);
    conn->beginRead();

    std::string u1 = makeUser();
    std::string u2 = makeUser();

    assert(client.createAccount(u1, "pw") && "Failed to create first account");
    assert(client.createAccount(u2, "pw") && "Failed to create second account");

    Logger::log("[Test] CreateAccountRequest passed\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// LOGIN TEST
// ===================================================

void testLoginRequest() {
    Logger::log("\n[Test] Running testLoginRequest...");

    ClientTestContext ctx;
    auto conn = TcpConnection::create(ctx.io(), nullptr);
    assert(conn->connect("127.0.0.1", 5555));

    Client client(conn);
    conn->beginRead();

    std::string user = makeUser();
    assert(client.createAccount(user, "pw"));
    assert(client.login(user, "pw"));

    Logger::log("[Test] LoginRequest passed\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// SEND MESSAGE TEST
// ===================================================

void testSendMessageRequest() {
    Logger::log("\n[Test] Running testSendMessageRequest...");

    ClientTestContext ctx;

    // sender
    auto connA = TcpConnection::create(ctx.io(), nullptr);
    assert(connA->connect("127.0.0.1", 5555));
    Client sender(connA);
    connA->beginRead();

    // receiver
    auto connB = TcpConnection::create(ctx.io(), nullptr);
    assert(connB->connect("127.0.0.1", 5555));
    Client receiver(connB);
    connB->beginRead();

    std::string userA = makeUser();
    std::string userB = makeUser();

    assert(sender.createAccount(userA, "pw"));
    assert(receiver.createAccount(userB, "pw"));
    assert(sender.login(userA, "pw"));

    assert(sender.sendMessage(userB, "Hello!") &&
           "Failed to send message to valid user");

    Logger::log("[Test] SendMessageRequest passed\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// RECEIVE MESSAGE TEST
// ===================================================

void testReceiveMessageResponse() {
    Logger::log("\n[Test] Running testReceiveMessageResponse...");

    ClientTestContext ctx;

    auto connA = TcpConnection::create(ctx.io(), nullptr);
    auto connB = TcpConnection::create(ctx.io(), nullptr);
    assert(connA->connect("127.0.0.1", 5555));
    assert(connB->connect("127.0.0.1", 5555));

    Client sender(connA);
    Client receiver(connB);
    connA->beginRead();
    connB->beginRead();

    std::string userA = makeUser();
    std::string userB = makeUser();

    assert(sender.createAccount(userA, "pw") && "Failed createAccount(A)");
    assert(receiver.createAccount(userB, "pw") && "Failed createAccount(B)");
    assert(sender.login(userA, "pw") && "Login failed for sender");

    // send
    assert(sender.sendMessage(userB, "hello") && "Failed to send message");

    // fetch
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(receiver.login(userB, "pw") && "Login failed for receiver");
    assert(receiver.getMessages(userA) && "Receiver failed getMessages()");

    auto messages = receiver.getDecryptedMessages();
    // verify at least one message exists
    assert(!messages.empty() && "Receiver got no messages!");

    const std::string& last = messages.back();

    // verify last message has text
    assert(last.find("test_user_") != std::string::npos);
    assert(last.find("hello") != std::string::npos);

    Logger::log("[Test] ReceiveMessageResponse passed\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// DISCONNECTED CLIENT TEST
// ===================================================

void testHandleDisconnectedClient() {
    Logger::log("\n[Test] Running testHandleDisconnectedClient...");

    ClientTestContext ctx;

    auto conn = TcpConnection::create(ctx.io(), nullptr);
    assert(conn->connect("127.0.0.1", 5555));

    Client client(conn);
    conn->beginRead();

    conn->socket().close();

    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    assert(!conn->socket().is_open() && "Socket should be closed");

    Logger::log("[Test] HandleDisconnectedClient passed\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// MULTIPLE CLIENTS TEST
// ===================================================

void testMultipleClientsSimultaneousConnections() {
    Logger::log("\n[Test] Running testMultipleClientsSimultaneousConnections...");

    ClientTestContext ctx;

    for (int i = 0; i < 3; i++) {
        auto conn = TcpConnection::create(ctx.io(), nullptr);
        assert(conn->connect("127.0.0.1", 5555));
        conn->beginRead();
    }

    Logger::log("[Test] MultipleClientsSimultaneousConnections passed\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// Main Entry
// ===================================================

int main() {
    Logger::log("=============================\n");
    Logger::log(" Running Network Unit Tests\n");
    Logger::log("=============================\n");

    resetUsers();

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
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    Logger::log("\nAll tests executed.\n");

    if (serverThread.joinable())
        serverThread.detach();

    return 0;
}