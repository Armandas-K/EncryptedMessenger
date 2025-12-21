#include "network/tcpServer.h"
#include <iostream>

#include "utils/Logger.h"

TcpServer::TcpServer(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      storage_(),
      messageHandler_(this, storage_)
{
    Logger::log("[TcpServer] Listening on port " + std::to_string(port));
    startAccept();
}

void TcpServer::startAccept() {
    // create a new connection object for the next incoming client.
    auto new_connection = TcpConnection::create(io_context_, this);

    // asynchronously wait for a new client to connect.
    acceptor_.async_accept(
        new_connection->socket(),
        [this, new_connection](const std::error_code& error) {
            handleAccept(new_connection, error);
        }
    );
}

void TcpServer::handleAccept(TcpConnection::pointer new_connection, const std::error_code& error) {
    if (!error) {
        Logger::log("[TcpServer] New connection accepted.\n");
        active_connections_.push_back(new_connection);
        new_connection->beginRead();
    } else {
        std::cerr << "[TcpServer] Accept error: " << error.message() << std::endl;
    }

    // continue accepting next connections
    startAccept();
}

void TcpServer::handleAction(TcpConnection::pointer connection, const nlohmann::json& message) {
    std::string action = message.value("action", "");

    if (action == "create_account") {
        handleCreateAccount(connection, message);
    } else if (action == "login") {
        handleLogin(connection, message);
    } else if (action == "send_message") {
        handleSendMessage(connection, message);
    } else if (action == "get_conversations") {
        handleGetConversations(connection, message);
    } else if (action == "get_messages") {
        handleGetMessages(connection, message);
    } else {
        std::cerr << "[TcpServer] Unknown action: " << action << std::endl;
    }
}

void TcpServer::handleCreateAccount(
        TcpConnection::pointer connection,
        const nlohmann::json& data) {
    std::string username = data.value("username", "");
    std::string password_hash = data.value("password_hash", "");

    if (username.empty() || password_hash.empty()) {
        connection->send(R"({"status":"error","message":"Missing credentials"})");
        return;
    }

    // atomic operation start
    std::lock_guard<std::mutex> guard(storage_.mutex());

    if (storage_.userExists_NoLock(username)) {
        connection->send(R"({"status":"error","message":"User already exists"})");
        return;
    }

    // write user to json
    if (!storage_.createUser_NoLock(username, password_hash)) {
        connection->send(R"({"status":"error","message":"Failed to create user"})");
        return;
    }

    // generate RSA key files
    if (!storage_.createUserKeyFiles_NoLock(username)) {
        // rollback user keys and json entry
        storage_.deleteUserKeys_NoLock(username);
        storage_.deleteUserJson_NoLock(username);
        storage_.saveUser_NoLock();
        connection->send(R"({"status":"error","message":"Failed to create user key files"})");
        return;
    }

    // Success
    connection->send(R"({"status":"success","message":"Account created"})");
}

void TcpServer::handleLogin(TcpConnection::pointer connection, const nlohmann::json& data) {
    std::string username = data.value("username", "");
    std::string password_hash = data.value("password_hash", "");

    if (!storage_.userExists(username)) {
        connection->send(R"({"status":"error","message":"Invalid username"})");
        return;
    }

    if (!storage_.loginUser(username, password_hash)) {
        connection->send(R"({"status":"error","message":"Invalid password"})");
        return;
    }
    // assign username to connection instance
    connection->setUsername(username);

    connection->send(R"({"status":"success","message":"Login successful"})");
}

void TcpServer::handleGetConversations(
    TcpConnection::pointer connection,
    const nlohmann::json& data) {
    messageHandler_.fetchConversations(connection);
}

void TcpServer::handleSendMessage(TcpConnection::pointer connection, const nlohmann::json& data) {
    std::string to = data.value("to", "");
    std::string message = data.value("message", "");

    if (to.empty() || message.empty()) {
        connection->send(R"({"status":"error","message":"Invalid message format"})");
        return;
    }

    messageHandler_.processMessage(connection, to, message);
}

void TcpServer::handleGetMessages(
    TcpConnection::pointer connection,
    const nlohmann::json& data) {
    std::string withUser = data.value("with", "");

    if (withUser.empty()) {
        connection->send(R"({"status":"error","message":"Missing username"})");
        return;
    }

    // query storage
    messageHandler_.fetchMessages(connection, withUser);
}

void TcpServer::removeConnection(TcpConnection::pointer connection) {
    auto it = std::find(active_connections_.begin(), active_connections_.end(), connection);
    if (it != active_connections_.end()) {
        active_connections_.erase(it);
        Logger::log("[TcpServer] Connection removed. Active connections: "
                  + std::to_string(active_connections_.size()));
    }
}