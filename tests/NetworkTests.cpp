#include "network/tcpServer.h"
#include "network/tcpConnection.h"
#include "client/Client.h"
#include <asio.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include "utils/ClientTestContext.h"
#include "utils/Logger.h"

// helpers
static void logTest(const std::string& name) {
    Logger::log("\n[Test] Running " + name);
}
static void passTest(const std::string& name) {
    Logger::log("[Test] " + name + " passed");
}

// ===================================================
// Delete Old User Data
// ===================================================

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
    logTest("Create Account");

    ClientTestContext ctx;

    auto conn = TcpConnection::create(ctx.io(), nullptr);
    assert(conn->connect("127.0.0.1", 5555));

    Client client(conn);
    conn->beginRead();

    std::string u1 = makeUser();
    std::string u2 = makeUser();

    assert(client.createAccount(u1, "pw") && "Failed to create first account");
    assert(client.createAccount(u2, "pw") && "Failed to create second account");

    passTest("Create Account");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// LOGIN TEST
// ===================================================

void testLoginRequest() {
    logTest("Login Request");

    ClientTestContext ctx;
    auto conn = TcpConnection::create(ctx.io(), nullptr);
    assert(conn->connect("127.0.0.1", 5555));

    Client client(conn);
    conn->beginRead();

    std::string user = makeUser();
    assert(client.createAccount(user, "pw"));
    assert(client.login(user, "pw"));

    passTest("Login Request");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// GET CONVERSATIONS TEST
// ===================================================

void testGetConversations() {
    logTest("Get Conversations");

    ClientTestContext ctx;
    auto connA = TcpConnection::create(ctx.io(), nullptr);
    auto connB = TcpConnection::create(ctx.io(), nullptr);
    assert(connA->connect("127.0.0.1", 5555));
    assert(connB->connect("127.0.0.1", 5555));

    Client clientA(connA);
    Client clientB(connB);
    connA->beginRead();
    connB->beginRead();

    std::string userA = makeUser();
    std::string userB = makeUser();

    // create accounts
    assert(clientA.createAccount(userA, "pw"));
    assert(clientB.createAccount(userB, "pw"));

    // login A
    assert(clientA.login(userA, "pw"));

    // get empty conversation list
    assert(clientA.getConversations() && "getConversations failed");

    auto convs = clientA.getCachedConversations();
    assert(convs.empty() && "Conversations should be empty initially");

    assert(clientA.sendMessage(userB, "hello from A"));
    assert(clientB.login(userB, "pw"));

    // get conversation list after sending message
    assert(clientA.getConversations() && "getConversations failed after message");

    // check if not empty
    convs = clientA.getCachedConversations();
    assert(!convs.empty() && "Conversation list should not be empty");

    // check userB appears
    bool found = false;
    for (const auto& c : convs) {
        if (c == userB) {
            found = true;
            break;
        }
    }
    assert(found && "Conversation with userB not found");

    passTest("Get Conversations");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// SEND MESSAGE TEST
// ===================================================

void testSendMessageRequest() {
    logTest("Send MessageRequest");

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

    passTest("Send MessageRequest");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// RECEIVE MESSAGE TEST
// ===================================================

void testReceiveMessageResponse() {
    logTest("Receive Message Response");

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

    passTest("Receive Message Response");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// DISCONNECTED CLIENT TEST
// ===================================================

void testHandleDisconnectedClient() {
    logTest("Handle Disconnected Client");

    ClientTestContext ctx;

    auto conn = TcpConnection::create(ctx.io(), nullptr);
    assert(conn->connect("127.0.0.1", 5555));

    Client client(conn);
    conn->beginRead();

    conn->socket().close();

    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    assert(!conn->socket().is_open() && "Socket should be closed");

    passTest("Handle Disconnected Client");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ===================================================
// MULTIPLE CLIENTS TEST
// ===================================================

void testMultipleClientConnections() {
    logTest("Multiple Client Connections");

    ClientTestContext ctx;

    for (int i = 0; i < 3; i++) {
        auto conn = TcpConnection::create(ctx.io(), nullptr);
        assert(conn->connect("127.0.0.1", 5555));
        conn->beginRead();
    }

    passTest("Multiple Client Connections");
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
    testGetConversations();
    testSendMessageRequest();
    testReceiveMessageResponse();
    testHandleDisconnectedClient();
    testMultipleClientConnections();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    Logger::log("\nAll network tests executed");

    resetUsers();

    if (serverThread.joinable())
        serverThread.detach();

    return 0;
}